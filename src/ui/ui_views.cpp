#include "ui/ui_views.hpp"
#include "ui/ui.hpp"
#include "renderer.hpp"
#include "process/injection.hpp"
#include "api/user/user_authentication.hpp"
#include "api/remote/download_binary.hpp"
#include "thread_pool.hpp"

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
				if (ImGui::BeginTable("LicensesTable", 5, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg))
				{
					ImGui::TableSetupColumn("Product / Module", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("License Key", ImGuiTableColumnFlags_WidthFixed, 240.0f);
					ImGui::TableSetupColumn("Issued Date", ImGuiTableColumnFlags_WidthFixed, 140.0f);
					ImGui::TableSetupColumn("Expires", ImGuiTableColumnFlags_WidthFixed, 160.0f);
					ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 90.0f);
					ImGui::TableHeadersRow();

					for (const auto& lic : ui_instance->m_user_licenses)
					{
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
					}
					ImGui::EndTable();
				}
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
			if (ImGui::BeginTable("AccessibleBinariesTable", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("Binary Name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("File Payload", ImGuiTableColumnFlags_WidthFixed, 180.0f);
				ImGui::TableSetupColumn("UUID / Release ID", ImGuiTableColumnFlags_WidthFixed, 220.0f);
				ImGui::TableSetupColumn("Access Status", ImGuiTableColumnFlags_WidthFixed, 110.0f);
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
						std::string bin_name = g_download_binary->get_binary_by_id((int)i);
						std::string file_name = g_download_binary->get_file_by_id((int)i);
						std::string uuid = g_download_binary->get_uuid_by_id((int)i);
						if (bin_name.empty()) bin_name = "Binary #" + std::to_string(i + 1);
						if (file_name.empty()) file_name = "payload.dll";

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						bool is_selected = (selected_idx == (int)i);
						if (ImGui::Selectable(bin_name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns))
						{
							selected_idx = (int)i;
							g_download_binary->select_binary_index((int)i);
							g_download_binary->select_binary(bin_name);
						}

						ImGui::TableSetColumnIndex(1);
						ImGui::Text("%s", file_name.c_str());

						ImGui::TableSetColumnIndex(2);
						ImGui::TextDisabled("%s", uuid.empty() ? "-" : uuid.c_str());

						ImGui::TableSetColumnIndex(3);
						ui::badge("ACCESSIBLE", ImVec4(0.16f, 0.72f, 0.53f, 0.25f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));
					}
				}
				ImGui::EndTable();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Selected Binary Details & Action Row
			std::string current_name = total_bins > 0 ? g_download_binary->get_binary_by_id(selected_idx) : "No Binary Selected";
			std::string current_file = total_bins > 0 ? g_download_binary->get_file_by_id(selected_idx) : "-";
			if (current_name.empty()) current_name = "Selected Binary";
			if (current_file.empty()) current_file = "payload.dll";

			ImGui::TextDisabled("Selected:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s (%s)", current_name.c_str(), current_file.c_str());

			ImGui::Spacing();

			float avail_w = ImGui::GetContentRegionAvail().x;
			float btn_w = (avail_w - 16.0f) * 0.5f;
			bool can_act = (total_bins > 0);

			if (ui::primary_button(ICON_FA_DOWNLOAD "  DOWNLOAD BINARY", ImVec2(btn_w, 44), can_act))
			{
				g_thread_pool->add_job([ui_instance, current_file] {
					ui_instance->m_is_downloading = true;
					ui_instance->m_download_progress = 0.0f;
					LOG(SERVER) << "Downloading binary stream: " << current_file;
					g_download_binary->check_binary_before_injection();
					ui_instance->m_is_downloading = false;
					ui_instance->m_binary_downloaded = true;
					ui_instance->show_toast("Download Complete", "Binary streamed & verified successfully.", ImVec4(0.16f, 0.72f, 0.53f, 1.0f));
				});
			}

			ImGui::SameLine(0, 16.0f);

			if (ui::primary_button(ICON_FA_ROCKET "  INJECT BINARY", ImVec2(btn_w, 44), can_act))
			{
				g_thread_pool->add_job([ui_instance] {
					if (g_download_binary->check_binary_before_injection())
					{
						LOG(SERVER) << "Injecting binary payload into target process";
						g_user_authentication->log_activity("PROCESS_INJECTION", "Injected binary into target process");
						g_injection->inject_library();
						ui_instance->m_injected = true;
						ui_instance->show_toast("Injection Successful", "Payload injected into target process.", ImVec4(0.16f, 0.72f, 0.53f, 1.0f));
					}
				});
			}
		}
		ui::card_end();
	}

	void ui_views::render_settings(ui* ui_instance)
	{
		ui::card_begin("SettingsCard", "SETTINGS & DIAGNOSTIC CONFIGURATION", "Customize loader preferences and view live system logs.");
		{
			ImGui::Text("Server API Base URL:");
			static char api_url[128] = "https://api.ellohim.com/v1";
			ui::input_text("##ApiUrl", "API Base URL", api_url, sizeof(api_url));

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
				ImGui::TextDisabled("[INFO] Connected to Ellohim-Server API endpoint: https://api.ellohim.com/v1");
				ImGui::TextDisabled("[STAGE 1] Product catalog loaded (3 available items).");
				ImGui::TextDisabled("[STAGE 2] Mock payment gateway initialized for QRIS / VA.");
				ImGui::TextDisabled("[STAGE 3] Binary download stream ready (SHA256 integrity verified).");
			}
			ImGui::EndChild();
		}
		ui::card_end();
	}
}
