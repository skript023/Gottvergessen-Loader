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
			size_t total_bins = g_download_binary ? g_download_binary->binaries_size() : 0;

			// Table of Accessible Binaries
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12.0f, 8.0f));
			if (ImGui::BeginTable("AccessibleBinariesTable", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollX))
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

						std::string bin_name = g_download_binary->get_binary_by_id((int)i);
						std::string file_name = g_download_binary->get_file_by_id((int)i);
						std::string version_val = g_download_binary->get_version_by_id((int)i);
						if (bin_name.empty()) bin_name = "Binary #" + std::to_string(i + 1);
						if (file_name.empty()) file_name = "payload.dll";

						std::string label = bin_name + "##" + std::to_string(i);

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						bool is_selected = (selected_idx == (int)i);
						if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns))
						{
							selected_idx = (int)i;
							g_download_binary->select_binary_index((int)i);
							g_download_binary->select_binary(bin_name);
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
			std::string current_name = total_bins > 0 ? g_download_binary->get_binary_by_id(selected_idx) : "No Binary Selected";
			std::string current_file = total_bins > 0 ? g_download_binary->get_file_by_id(selected_idx) : "-";
			if (current_name.empty()) current_name = "Selected Binary";
			if (current_file.empty()) current_file = "payload.dll";

			ImGui::TextDisabled("Selected:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s (%s)", current_name.c_str(), current_file.c_str());

			ImGui::Spacing();

			// Target Process Selector / Input
			static char target_proc_buf[64] = "notepad.exe";
			if (g_download_binary)
			{
				std::string current_target = g_download_binary->injection_target();
				if (!current_target.empty() && strcmp(target_proc_buf, "notepad.exe") == 0)
				{
					strncpy_s(target_proc_buf, current_target.c_str(), sizeof(target_proc_buf) - 1);
				}
			}

			ImGui::Text("Target Process Name:");
			ImGui::SetNextItemWidth(260.0f);
			if (ImGui::InputText("##TargetProcessInput", target_proc_buf, sizeof(target_proc_buf)))
			{
				if (g_download_binary)
				{
					g_download_binary->set_target_process(target_proc_buf);
				}
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(e.g., notepad.exe, GTA5.exe)");

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
					bool ok = g_download_binary->download(g_download_binary->get_selected_uuid().empty() ? current_file : g_download_binary->get_selected_uuid(), g_download_binary->get_binary_name());
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
				if (g_download_binary)
				{
					g_download_binary->set_target_process(target_proc_buf);
				}

				g_thread_pool->add_job([ui_instance] {
					if (g_download_binary->check_binary_before_injection())
					{
						LOG(SERVER) << "Injecting binary payload into target process: " << g_download_binary->injection_target();
						g_user_authentication->log_activity("PROCESS_INJECTION", "Injected binary into target process: " + g_download_binary->injection_target());
						if (g_injection->inject_library())
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
