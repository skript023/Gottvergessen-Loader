#include "thread_pool.hpp"
#include "views/views.hpp"

#include <imgui.h>
#include <format>

#include "process/injection.hpp"
#include "api/remote/download_binary.hpp"
#include "api/user/user_authentication.hpp"
#include "logger.hpp"

namespace gottvergessen
{
	void views::injection_view()
	{
		ImVec2 content_size = ImGui::GetContentRegionAvail();
		float window_width = content_size.x;

		// =========================================================================
		// 1. DASHBOARD HEADER BANNER (Profile Summary Bar)
		// =========================================================================
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.12f, 0.16f, 0.85f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));

		if (ImGui::BeginChild("HeaderProfileBanner", ImVec2(0, 52), true, ImGuiWindowFlags_NoScrollbar))
		{
			// Left side: User Avatar / Badge & Greeting
			ImGui::SetCursorPos(ImVec2(12, 10));
			ImGui::TextColored(ImVec4(0.38f, 0.72f, 1.00f, 1.00f), "[PROFILE]");

			ImGui::SameLine(0, 8);
			std::string display_name = g_user_authentication->get_fullname();
			if (display_name.empty()) display_name = g_user_authentication->get_username();

			ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.00f), "Welcome back, %s", display_name.c_str());

			ImGui::SameLine(0, 15);
			// Role Badge
			std::string role = g_user_authentication->get_role();
			ImVec4 role_color = (role == "admin" || role == "Admin") ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(0.3f, 0.8f, 0.4f, 1.0f);
			ImGui::TextColored(role_color, "[%s]", role.empty() ? "Customer" : role.c_str());

			// Right side: Logout Button
			float logout_btn_width = 75.0f;
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - logout_btn_width - 12, 10));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 0.70f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.25f, 0.25f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.15f, 0.15f, 1.00f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			if (ImGui::Button("Logout", ImVec2(logout_btn_width, 30)))
			{
				g_user_authentication->logout();
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(3);
		}
		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();

		ImGui::Spacing();

		// =========================================================================
		// 2. TWO-COLUMN DASHBOARD CONTENT (Profile Info Left | Software Catalog Right)
		// =========================================================================
		float gap = 12.0f;
		float col_width = (window_width - gap) * 0.5f;

		// -------------------------------------------------------------------------
		// LEFT COLUMN: ACCOUNT PROFILE & SYSTEM INFORMATION
		// -------------------------------------------------------------------------
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.10f, 0.13f, 0.75f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 14));

		if (ImGui::BeginChild("LeftProfileColumn", ImVec2(col_width, 240), true))
		{
			ImGui::TextColored(ImVec4(0.38f, 0.72f, 1.00f, 1.00f), "Account Information");
			ImGui::Separator();
			ImGui::Spacing();

			// Profile details
			ImGui::TextDisabled("Username:");
			ImGui::SameLine(120);
			ImGui::Text("%s", g_user_authentication->get_username().c_str());

			ImGui::TextDisabled("Full Name:");
			ImGui::SameLine(120);
			ImGui::Text("%s", g_user_authentication->get_fullname().c_str());

			ImGui::TextDisabled("Ownership:");
			ImGui::SameLine(120);
			std::string ownership_info = g_user_authentication->owned_product_info(g_user_authentication->owned_product());
			ImGui::TextColored(ImVec4(0.95f, 0.80f, 0.30f, 1.00f), "%s", ownership_info.c_str());

			ImGui::TextDisabled("License Expiry:");
			ImGui::SameLine(120);
			ImGui::Text("%s", g_user_authentication->ownership_expiry_date().c_str());

			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.38f, 0.72f, 1.00f, 1.00f), "System Diagnostics");
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::TextDisabled("Computer Name:");
			ImGui::SameLine(120);
			ImGui::Text("%s", g_user_authentication->get_computer_name().c_str());

			ImGui::TextDisabled("CPU Threads:");
			ImGui::SameLine(120);
			ImVec4 proc_color = g_user_authentication->get_thread_count() <= 2 ? ImVec4(1.00f, 0.30f, 0.30f, 1.00f) : ImVec4(0.30f, 0.90f, 0.40f, 1.00f);
			ImGui::TextColored(proc_color, "%d Threads", g_user_authentication->get_thread_count());

			ImGui::TextDisabled("Loader Status:");
			ImGui::SameLine(120);
			ImGui::TextColored(ImVec4(0.30f, 0.90f, 0.40f, 1.00f), "Online (Synced)");
		}
		ImGui::EndChild();

		ImGui::SameLine(0, gap);

		// -------------------------------------------------------------------------
		// RIGHT COLUMN: EL LOHIM SOFTWARE CATALOG & INJECTION WORKFLOW
		// -------------------------------------------------------------------------
		if (ImGui::BeginChild("RightWorkflowColumn", ImVec2(col_width, 240), true))
		{
			ImGui::TextColored(ImVec4(0.38f, 0.72f, 1.00f, 1.00f), "Software Release Catalog");
			ImGui::Separator();
			ImGui::Spacing();

			static int selected_index = 0;

			ImGui::TextDisabled("Select Target Product:");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

			std::string current_name = g_download_binary->get_binary_by_id(selected_index);
			if (current_name.empty()) current_name = "Select Software Release";

			if (ImGui::BeginCombo("##BinaryListCombo", current_name.c_str()))
			{
				size_t total_binaries = g_download_binary->binaries_size();
				for (size_t i = 0; i < total_binaries; i++)
				{
					std::string bin_name = g_download_binary->get_binary_by_id((int)i);
					if (bin_name.empty()) continue;

					bool is_selected = (selected_index == (int)i);
					if (ImGui::Selectable(bin_name.c_str(), is_selected))
					{
						selected_index = (int)i;
						g_download_binary->select_binary(bin_name);
					}
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Spacing();
			
			// Selected Package Info
			std::string selected_file = g_download_binary->get_file_by_id(selected_index);
			if (selected_file.empty()) selected_file = "binary_package.dll";

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.14f, 0.18f, 0.60f));
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
			if (ImGui::BeginChild("PackageDetailBox", ImVec2(0, 56), true))
			{
				ImGui::TextDisabled("Target Package:");
				ImGui::SameLine(110);
				ImGui::TextColored(ImVec4(0.90f, 0.90f, 0.95f, 1.00f), "%s", current_name.c_str());

				ImGui::TextDisabled("File Payload:");
				ImGui::SameLine(110);
				ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.75f, 1.00f), "%s", selected_file.c_str());
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			ImGui::Spacing();

			// Workflow Action Buttons
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.48f, 0.85f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.58f, 0.95f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.38f, 0.70f, 1.00f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

			float avail_w = ImGui::GetContentRegionAvail().x;
			if (ImGui::Button("START LOADER & INJECT", ImVec2(avail_w, 36)))
			{
				g_thread_pool->add_job([] {
					if (g_download_binary->check_binary_before_injection())
					{
						LOG(SERVER) << "Injecting Library package";
						g_injection->inject_library();
					}
				});
			}

			ImGui::PopStyleVar();
			ImGui::PopStyleColor(3);
		}
		ImGui::EndChild();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
	}
}