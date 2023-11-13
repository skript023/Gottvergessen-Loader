#include "views/views.hpp"
#include "thread_pool.hpp"

#include <imgui.h>

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
		auto width = ImGui::GetWindowSize();

		// figure out where we need to move the button to. It's good if you understand why this formula works!
		ImVec2 centre_position_for_button((width.x - button_size.x) / 2, (width.y - button_size.y) / 2);
		ImVec2 center_position_for_input((width.x - input_size) / 2, (width.y - input_size) / 2);

		ImGui::PushItemWidth(input_size);

		ImGui::SetCursorPosX(center_position_for_input.x);
		ImGui::Text("Username");
		ImGui::SetCursorPosX(center_position_for_input.x);
		ImGui::InputText(xorstr("##Username"), g_user_authentication->username, IM_ARRAYSIZE(g_user_authentication->username));
		
		ImGui::SetCursorPosX(center_position_for_input.x);
		ImGui::Text("Password");
		ImGui::SetCursorPosX(center_position_for_input.x);
		ImGui::InputText(xorstr("##Password"), g_user_authentication->password, IM_ARRAYSIZE(g_user_authentication->password), ImGuiInputTextFlags_Password);
		
		ImGui::PopItemWidth();

		ImGui::SetCursorPosX(centre_position_for_button.x);
		if (ImGui::Button(xorstr("Login"), button_size))
		{
			g_thread_pool->add_job([] {
				if (g_user_authentication->login(g_user_authentication->username, g_user_authentication->password))
				{
					g_costume_loader->execute();
					memset(g_user_authentication->password, NULL, sizeof(g_user_authentication->password));

					LOG(HACKER) << g_user_authentication->get_message();
				}
			});
		}
	}
}