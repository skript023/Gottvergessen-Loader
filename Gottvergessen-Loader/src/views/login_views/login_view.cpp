#include "views/views.hpp"
#include "thread_pool.hpp"
#include "docking_imgui/imgui.h"

#include "process/injection.hpp"
#include "api/remote/download_binary.hpp"
#include "api/user/user_authentication.hpp"
#include "api/costume_loader/costume_loader.hpp"

namespace gottvergessen
{
	void views::login_view()
	{
		ImVec2 button_size = ImVec2{ 120, 0 };
		constexpr float input_size = 200.f;

		// obtain width of window
		float width = ImGui::GetWindowSize().x;

		// figure out where we need to move the button to. It's good if you understand why this formula works!
		float centre_position_for_button = (width - button_size.x) / 2;
		float center_position_for_input = (width - input_size) / 2;

		ImGui::PushItemWidth(input_size);

		ImGui::SetCursorPosX(center_position_for_input);
		ImGui::Text("Username");
		ImGui::SetCursorPosX(center_position_for_input);
		ImGui::InputText(xorstr("##Username"), g_user_authentication->username, IM_ARRAYSIZE(g_user_authentication->username));
		
		ImGui::SetCursorPosX(center_position_for_input);
		ImGui::Text("Password");
		ImGui::SetCursorPosX(center_position_for_input);
		ImGui::InputText(xorstr("##Password"), g_user_authentication->password, IM_ARRAYSIZE(g_user_authentication->password), ImGuiInputTextFlags_Password);
		
		ImGui::PopItemWidth();

		ImGui::SetCursorPosX(centre_position_for_button);
		if (ImGui::Button(xorstr("Login"), button_size))
		{
			g_thread_pool->add_job([] {
				if (g_user_authentication->login(g_user_authentication->username, g_user_authentication->password))
				{
					g_costume_loader->execute();
					LOG(HACKER) << g_user_authentication->get_message();
				}
			});
		}
	}
}