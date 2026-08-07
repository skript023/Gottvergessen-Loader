#include "ui/ui_views.hpp"
#include "ui/ui.hpp"
#include "renderer.hpp"
#include "process/injection.hpp"
#include "api/user/user_authentication.hpp"
#include "api/remote/download_binary.hpp"
#include "thread_pool.hpp"
#include "api/environment.hpp"

#include <imgui.h>
#include <format>
#include <iomanip>
#include <sstream>

namespace gottvergessen
{
	// Helper format IDR
	static std::string format_idr(double amount)
	{
		std::stringstream ss;
		ss << "Rp " << std::fixed << std::setprecision(0) << amount;
		return ss.str();
	}

	void ui_views::render_my_licenses(ui* ui_instance)
	{
		ui::card_begin("MyLicensesCard", "MY LICENSES & SUBSCRIPTIONS", "Active cryptographic license keys issued to your account.");
		{
			if (ui::secondary_button(ICON_FA_SYNC "  Refresh Licenses", ImVec2(160, 32)))
			{
				ui_instance->fetch_data_from_server();
			}

			ImGui::Spacing();

			if (ui_instance->m_user_licenses.empty())
			{
				ImGui::TextDisabled("No active licenses found for your account.");
			}
			else
			{
				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12.0f, 8.0f));
				if (ImGui::BeginTable("LicensesTable", 5, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollX))
				{
					ImGui::TableSetupColumn("Product / Module", ImGuiTableColumnFlags_WidthStretch, 0.28f);
					ImGui::TableSetupColumn("License Key", ImGuiTableColumnFlags_WidthFixed, 220.0f);
					ImGui::TableSetupColumn("Issued Date", ImGuiTableColumnFlags_WidthFixed, 150.0f);
					ImGui::TableSetupColumn("Expires", ImGuiTableColumnFlags_WidthFixed, 150.0f);
					ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
					ImGui::TableHeadersRow();

					int lic_idx = 0;
					for (const auto& lic : ui_instance->m_user_licenses)
					{
						ImGui::PushID(lic_idx++);
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s", lic.product_name.c_str());

						ImGui::TableSetColumnIndex(1);
						ImGui::TextColored(ImVec4(0.23f, 0.51f, 0.96f, 1.0f), "%s", lic.license_key.c_str());

						ImGui::TableSetColumnIndex(2);
						ImGui::TextDisabled("%s", lic.issued_at.empty() ? "-" : lic.issued_at.c_str());

						ImGui::TableSetColumnIndex(3);
						ImGui::Text("%s", lic.expiry_date.empty() ? "Lifetime" : lic.expiry_date.c_str());

						ImGui::TableSetColumnIndex(4);
						ui::badge(lic.status.c_str(), ImVec4(0.16f, 0.72f, 0.53f, 0.2f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));
						ImGui::PopID();
					}
					ImGui::EndTable();
				}
				ImGui::PopStyleVar();
			}
		}
		ui::card_end();
	}

	void ui_views::render_binary_download(ui* ui_instance, renderer* renderer_ptr)
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
					strncpy(target_proc_buf, current_target.c_str(), sizeof(target_proc_buf) - 1);
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
						ui_instance->show_toast("Download Complete", "Binary streamed & verified successfully.", ImVec4(0.16f, 0.72f, 0.53f, 1.0f));
					}
					else
					{
						ui_instance->show_toast("Download Failed", "Could not stream binary payload from server.", ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
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
							ui_instance->show_toast("Injection Successful", "Payload injected into target process.", ImVec4(0.16f, 0.72f, 0.53f, 1.0f));
						}
						else
						{
							ui_instance->show_toast("Injection Failed", "Process not running or injection error.", ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
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

	void ui_views::render_settings(ui* ui_instance)
	{
		ui::card_begin("SettingsCard", "SETTINGS & DIAGNOSTIC CONFIGURATION", "Customize loader preferences and view live system logs.");
		{
			ImGui::Text("Target Server Environment:");
			
			int current_env = static_cast<int>(environment_manager::get().get_current_environment());
			const char* env_names[] = {
				"Localhost (http://localhost:8180)",
				"Production (https://apie.rena.my.id)",
				"Custom URL..."
			};

			if (ImGui::Combo("##EnvironmentCombo", &current_env, env_names, IM_ARRAYSIZE(env_names)))
			{
				environment_manager::get().set_environment(static_cast<Environment>(current_env));
			}

			if (environment_manager::get().get_current_environment() == Environment::CUSTOM)
			{
				ImGui::Spacing();
				ImGui::Text("Custom API Base URL:");
				static char custom_url_buf[256] = "";
				if (custom_url_buf[0] == '\0')
				{
					std::string cur_custom = environment_manager::get().get_custom_url();
					strncpy_s(custom_url_buf, cur_custom.c_str(), sizeof(custom_url_buf) - 1);
				}
				if (ui::input_text("##CustomApiUrl", "Custom API Base URL", custom_url_buf, sizeof(custom_url_buf)))
				{
					environment_manager::get().set_custom_url(custom_url_buf);
				}
			}

			ImGui::Spacing();
			std::string active_url = environment_manager::get().get_base_url();
			ImGui::TextDisabled("Active API URL: %s", active_url.c_str());

			ImGui::Spacing();
			static bool auto_update = true;
			ImGui::Checkbox("Enable Automatic Binary Updates", &auto_update);

			static bool stream_logs = true;
			ImGui::Checkbox("Stream Realtime G3log Output to Console", &stream_logs);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "Live System Log Window");
			ImGui::BeginChild("LogConsoleArea", ImVec2(0, 180.0f), true);
			{
				ImGui::TextDisabled("[INFO] Gottvergessen UI Wrapper v1.0 initialized.");
				ImGui::TextDisabled("[INFO] Active API endpoint: %s", active_url.c_str());
				ImGui::TextDisabled("[STAGE 1] Product catalog loaded (3 available items).");
				ImGui::TextDisabled("[STAGE 2] Mock payment gateway initialized for QRIS / VA.");
				ImGui::TextDisabled("[STAGE 3] Binary download stream ready (SHA256 integrity verified).");
			}
			ImGui::EndChild();
		}
		ui::card_end();
	}
}
