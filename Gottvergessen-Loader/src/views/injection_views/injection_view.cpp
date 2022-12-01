#include "thread_pool.hpp"
#include "views/views.hpp"
#include "docking_imgui/imgui.h"

#include "process/injection.hpp"
#include "api/remote/download_binary.hpp"
#include "api/user/user_authentication.hpp"


namespace gottvergessen
{
	void views::injection_view()
	{
		ImVec2 button_size = ImVec2{ 120, 0 };
		// obtain width of window
		float width = ImGui::GetWindowSize().x;

		// figure out where we need to move the button to. It's good if you understand why this formula works!
		float centre_position_for_button = (width - button_size.x) / 2;

		ImGui::TextColored(g_download_binary->bin_current_version_machine() < g_download_binary->bin_latest_version_machine() ? ImVec4(255, 0, 0, 255) : ImVec4(0, 255, 0, 255), std::format("Current Version: {}", g_download_binary->bin_current_version()).c_str());
		ImGui::Text(std::format("Fullname: {}", g_user_authentication->get_fullname()).c_str());
		ImGui::Text(std::format("Ownership: {}", g_user_authentication->owned_product_info(g_user_authentication->owned_product())).c_str());
		ImGui::Text(std::format("Expired Date: {}", g_user_authentication->ownership_expiry_date()).c_str());
		ImGui::Text(std::format("Role: {}", g_user_authentication->get_role()).c_str());
		ImGui::Text(std::format("Computer Name: {}", g_user_authentication->get_computer_name()).c_str());
		ImGui::TextColored({ 0, 255, 0, 255 }, std::format("Processor Thread: {}", g_user_authentication->get_thread_count()).c_str());

		ImGui::SetCursorPosX(centre_position_for_button);

		if (ImGui::Button("Start Loader"))
		{
			g_thread_pool->add_job([] {
				if (g_download_binary->check_binary_before_injection())
				{
					g_injection->inject_library();
				}
			});
		}
	}
}