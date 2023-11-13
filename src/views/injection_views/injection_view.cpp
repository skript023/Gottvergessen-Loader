#include "thread_pool.hpp"
#include "views/views.hpp"

#include <imgui.h>

#include "process/injection.hpp"
#include "api/remote/download_binary.hpp"
#include "api/user/user_authentication.hpp"


namespace gottvergessen
{
	void views::injection_view()
	{
		ImVec2 button_size = ImVec2{ 120, 0 };
		constexpr float combo_size = 200.f;
		// obtain width of window
		float width = ImGui::GetWindowSize().x;

		// figure out where we need to move the button to. It's good if you understand why this formula works!
		float centre_position_for_button = (width - button_size.x) / 2;
		float center_position_for_combo = (width - combo_size) / 2;

		ImVec4 processor_color = g_user_authentication->get_thread_count() == 2 ? ImVec4(255, 0, 0, 255) : ImVec4(0, 255, 0, 255);

		if (g_download_binary->is_version_valid())
			ImGui::TextColored(ImVec4(0, 255, 0, 255), std::format("Current Version: {}", g_download_binary->loader_version()).c_str());
		ImGui::Text(std::format("Fullname: {}", g_user_authentication->get_fullname()).c_str());
		ImGui::Text(std::format("Ownership: {}", g_user_authentication->owned_product_info(g_user_authentication->owned_product())).c_str());
		ImGui::Text(std::format("Expired Date: {}", g_user_authentication->ownership_expiry_date()).c_str());
		ImGui::Text(std::format("Role: {}", g_user_authentication->get_role()).c_str());
		ImGui::Text(std::format("Computer Name: {}", g_user_authentication->get_computer_name()).c_str());
		//ImGui::Text(std::format("Computer UUID: {}", g_user_authentication->get_bios()).c_str());
		ImGui::TextColored(processor_color, std::format("Processor Thread: {}", g_user_authentication->get_thread_count()).c_str());

		ImGui::SetCursorPosX(center_position_for_combo);
		ImGui::Text("Select DLL");
		static int selected = 1;
		ImGui::PushItemWidth(combo_size);
		ImGui::SetCursorPosX(center_position_for_combo);
		if (ImGui::BeginCombo("## Binary List", g_download_binary->get_binary_by_id(selected).c_str()))
		{
			for (int i = 0; i < g_download_binary->binaries_size(); i++)
			{
				if (ImGui::Selectable(g_download_binary->get_binary_by_id(i).c_str(), i == selected))
				{
					selected = i;
					g_download_binary->select_binary(g_download_binary->get_binary_by_id(i));
				}
				if (i == selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::SetCursorPosX(centre_position_for_button);
		
		if (ImGui::Button("Start Loader"))
		{
			g_thread_pool->add_job([] {
				if (g_download_binary->check_binary_before_injection())
				{
					LOG(SERVER) << "Injecting Library";
					g_injection->inject_library();
				}
			});
		}
	}
}