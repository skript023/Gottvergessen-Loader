#include "ui/views/binary_launch_view.hpp"
#include "ui/ui.hpp"
#include "renderer.hpp"
#include "process/injection.hpp"
#include "api/user/user_authentication.hpp"
#include "api/remote/download_binary.hpp"
#include "thread_pool.hpp"

#include <imgui.h>
#include <string>
#include <cstring>

namespace gottvergessen
{
	void binary_launch_view::render_impl(ui* ui_instance, renderer* renderer_ptr)
	{
		ui::card_begin("BinaryLaunchCard", "ACCESSIBLE BINARIES & LAUNCH ENGINE", "Select accessible binary from server, download payload stream, and execute injection.");
		{
			ImGui::TextColored(ImVec4(0.38f, 0.72f, 1.00f, 1.00f), "Accessible Product Binaries List:");
			ImGui::Spacing();

			static int selected_idx = 0;
			size_t total_bins = download_binary::binaries_size();

			// Table of Accessible Binaries
			float row_h = 36.0f;
			float table_h = row_h * (total_bins > 0 ? (total_bins + 1.2f) : 2.5f);
			if (table_h > 230.0f) table_h = 230.0f;

			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12.0f, 8.0f));
			if (ImGui::BeginTable("AccessibleBinariesTable", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY, ImVec2(0.0f, table_h)))
			{
				ImGui::TableSetupColumn("Binary Name", ImGuiTableColumnFlags_WidthStretch, 0.40f);
				ImGui::TableSetupColumn("File Payload", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableHeadersRow();

				if (total_bins == 0)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextDisabled("No accessible binaries found for your account.");
				}
				else
				{
					for (size_t i = 0; i < total_bins; i++)
					{
						ImGui::PushID(static_cast<int>(i));

						std::string bin_name = download_binary::get_binary_by_id((int)i);
						std::string file_name = download_binary::get_file_by_id((int)i);
						std::string version_val = download_binary::get_version_by_id((int)i);
						if (bin_name.empty()) bin_name = "Binary #" + std::to_string(i + 1);
						if (file_name.empty()) file_name = "payload.dll";

						std::string label = bin_name + "##" + std::to_string(i);

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						bool is_selected = (selected_idx == (int)i);
						if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns))
						{
							selected_idx = (int)i;
							download_binary::select_binary_index((int)i);
							download_binary::select_binary(bin_name);
						}

						ImGui::TableSetColumnIndex(1);
						ImGui::Text("%s", file_name.c_str());

						ImGui::TableSetColumnIndex(2);
						ImGui::TextColored(ImVec4(0.35f, 0.70f, 1.00f, 1.0f), "%s", version_val.c_str());

						ImGui::TableSetColumnIndex(3);
						ui::badge("ACCESSIBLE", ImVec4(0.16f, 0.72f, 0.53f, 0.25f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));

						ImGui::PopID();
					}
				}
				ImGui::EndTable();
			}
			ImGui::PopStyleVar();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Selected Binary Details & Target Process Configuration
			std::string current_name = total_bins > 0 ? download_binary::get_binary_by_id(selected_idx) : "No Binary Selected";
			std::string current_file = total_bins > 0 ? download_binary::get_file_by_id(selected_idx) : "-";
			if (current_name.empty()) current_name = "Selected Binary";
			if (current_file.empty()) current_file = "payload.dll";

			ImGui::TextDisabled("Selected:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s (%s)", current_name.c_str(), current_file.c_str());

			ImGui::Spacing();

			// Target Process Selector / Input
			static char target_proc_buf[64] = "notepad.exe";
			static bool show_process_picker = false;
			static std::vector<process_info> cached_processes;
			static char filter_buf[64] = "";
			static int selected_proc_pid = 0;

			std::string current_target = download_binary::injection_target();
			if (!current_target.empty() && strcmp(target_proc_buf, "notepad.exe") == 0)
			{
				strncpy_s(target_proc_buf, current_target.c_str(), sizeof(target_proc_buf) - 1);
			}

			ImGui::Text("Target Process Name:");
			ImGui::SetNextItemWidth(260.0f);
			if (ImGui::InputText("##TargetProcessInput", target_proc_buf, sizeof(target_proc_buf)))
			{
				download_binary::set_target_process(target_proc_buf);
				injection::set_target_pid(0);
			}
			ImGui::SameLine();

			if (ImGui::Button(ICON_FA_LIST "  Select Process"))
			{
				show_process_picker = true;
				cached_processes = injection_method::get_running_processes();
				filter_buf[0] = '\0';
				selected_proc_pid = injection::get_target_pid();
			}

			// Modal Process Picker
			if (show_process_picker)
			{
				ImGui::OpenPopup("Select Process Modal");
			}

			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(620.0f, 420.0f), ImGuiCond_Appearing);

			if (ImGui::BeginPopupModal("Select Process Modal", &show_process_picker, ImGuiWindowFlags_NoResize))
			{
				ImGui::TextColored(ImVec4(0.35f, 0.70f, 1.00f, 1.0f), "Running Windows Processes");
				ImGui::TextDisabled("Select a target process to inject into. Zombie / terminating processes are auto-filtered.");
				ImGui::Spacing();

				// Filter and Refresh
				ImGui::SetNextItemWidth(340.0f);
				ImGui::InputTextWithHint("##ProcessFilter", ICON_FA_SEARCH " Search PID or Name...", filter_buf, sizeof(filter_buf));
				ImGui::SameLine();
				if (ImGui::Button(ICON_FA_SYNC " Refresh"))
				{
					cached_processes = injection_method::get_running_processes();
				}

				ImGui::Spacing();

				// Process Table
				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 6.0f));
				if (ImGui::BeginTable("ProcessPickerTable", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 240.0f)))
				{
					ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
					ImGui::TableSetupColumn("Process Name", ImGuiTableColumnFlags_WidthStretch, 0.50f);
					ImGui::TableSetupColumn("Arch", ImGuiTableColumnFlags_WidthFixed, 70.0f);
					ImGui::TableSetupColumn("Access Status", ImGuiTableColumnFlags_WidthFixed, 120.0f);
					ImGui::TableHeadersRow();

					std::string filter_str = filter_buf;
					std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);

					for (const auto& proc : cached_processes)
					{
						std::string proc_name_lower = proc.name;
						std::transform(proc_name_lower.begin(), proc_name_lower.end(), proc_name_lower.begin(), ::tolower);
						std::string pid_str = std::to_string(proc.pid);

						if (!filter_str.empty() && proc_name_lower.find(filter_str) == std::string::npos && pid_str.find(filter_str) == std::string::npos)
						{
							continue;
						}

						ImGui::PushID((int)proc.pid);
						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						bool is_sel = (selected_proc_pid == (int)proc.pid);
						std::string pid_label = pid_str + "##" + pid_str;
						if (ImGui::Selectable(pid_label.c_str(), is_sel, ImGuiSelectableFlags_SpanAllColumns))
						{
							selected_proc_pid = (int)proc.pid;
							strncpy_s(target_proc_buf, proc.name.c_str(), sizeof(target_proc_buf) - 1);
							download_binary::set_target_process(target_proc_buf);
							injection::set_target_pid((DWORD)proc.pid);
						}

						ImGui::TableSetColumnIndex(1);
						ImGui::Text("%s", proc.name.c_str());

						ImGui::TableSetColumnIndex(2);
						ImGui::TextDisabled("%s", proc.arch == "x64" ? "x64" : "x86");

						ImGui::TableSetColumnIndex(3);
						if (proc.is_accessible)
						{
							ui::badge("ACCESSIBLE", ImVec4(0.16f, 0.72f, 0.53f, 0.20f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));
						}
						else
						{
							ui::badge("NO ACCESS", ImVec4(0.85f, 0.20f, 0.20f, 0.20f), ImVec4(0.95f, 0.40f, 0.40f, 1.0f));
						}

						ImGui::PopID();
					}
					ImGui::EndTable();
				}
				ImGui::PopStyleVar();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				if (ui::primary_button("SELECT & CLOSE", ImVec2(160.0f, 34.0f)))
				{
					if (selected_proc_pid > 0)
					{
						injection::set_target_pid((DWORD)selected_proc_pid);
						download_binary::set_target_process(target_proc_buf);
					}
					show_process_picker = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ui::secondary_button("CANCEL", ImVec2(100.0f, 34.0f)))
				{
					show_process_picker = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::Spacing();

			float avail_w = ImGui::GetContentRegionAvail().x;
			float btn_w = (avail_w - 16.0f) * 0.5f;
			bool can_act = (total_bins > 0);

			if (ui::primary_button(ICON_FA_DOWNLOAD "  DOWNLOAD BINARY", ImVec2(btn_w, 44), can_act && !ui_instance->m_is_downloading))
			{
				g_thread_pool->add_job([ui_instance, current_file] {
					ui_instance->m_is_downloading = true;
					ui_instance->m_download_progress = 0.0f;
					LOG(SERVER) << "Downloading binary stream: " << current_file;
					bool ok = download_binary::download(download_binary::get_selected_uuid().empty() ? current_file : download_binary::get_selected_uuid(), download_binary::get_binary_name());
					ui_instance->m_is_downloading = false;
					if (ok)
					{
						ui_instance->m_binary_downloaded = true;
						ui::show_toast("Download Complete", "Binary streamed & verified successfully.", ImVec4(0.16f, 0.72f, 0.53f, 1.0f));
					}
					else
					{
						ui::show_toast("Download Failed", "Could not stream binary payload from server.", ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
					}
				});
			}

			ImGui::SameLine(0, 16.0f);

			if (ui::primary_button(ICON_FA_ROCKET "  INJECT BINARY", ImVec2(btn_w, 44), can_act && !ui_instance->m_is_downloading))
			{
				download_binary::set_target_process(target_proc_buf);

				g_thread_pool->add_job([ui_instance] {
					if (download_binary::check_binary_before_injection())
					{
						LOG(SERVER) << "Injecting binary payload into target process: " << download_binary::injection_target();
						user_authentication::log_activity("PROCESS_INJECTION", "Injected binary into target process: " + download_binary::injection_target());
						if (injection::inject_library())
						{
							ui_instance->m_injected = true;
							ui::show_toast("Injection Successful", "Payload injected into target process.", ImVec4(0.16f, 0.72f, 0.53f, 1.0f));
						}
						else
						{
							ui::show_toast("Injection Failed", "Process not running or injection error.", ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
						}
					}
				});
			}

			// =========================================================================
			// DOWNLOAD PROGRESS MODAL DIALOGUE
			// =========================================================================
			if (ui_instance->m_is_downloading)
			{
				ImGui::OpenPopup("Downloading Binary Stream");
			}

			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(420.0f, 160.0f));
			if (ImGui::BeginPopupModal("Downloading Binary Stream", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextColored(ImVec4(0.23f, 0.51f, 0.96f, 1.0f), ICON_FA_DOWNLOAD "  Streaming & Decrypting Payload...");
				ImGui::Spacing();
				ImGui::TextDisabled("Target File: %s", current_file.c_str());
				ImGui::Spacing();

				float pct = ui_instance->m_download_progress;
				char overlay[32];
				snprintf(overlay, sizeof(overlay), "%.0f%%", pct * 100.0f);
				ImGui::ProgressBar(pct, ImVec2(-1, 28), overlay);

				if (!ui_instance->m_is_downloading)
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}
		ui::card_end();
	}
}
