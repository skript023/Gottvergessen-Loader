#include "views/views.hpp"

#include "renderer.hpp"
#include "thread_pool.hpp"

#include <imgui.h>

#include "process/injection.hpp"
#include "api/remote/download_binary.hpp"
#include "api/user/user_authentication.hpp"
#include "api/environment.hpp"

namespace gottvergessen
{
	void views::login_view(renderer* renderer)
	{
		ImVec2 window_size = ImGui::GetWindowSize();
		float font_size = ImGui::GetFontSize();

		// 2. Skala dinamis berbasis proporsi window & skala font
		float target_width = window_size.x * 0.45f;
		
		float min_width = std::max(260.0f, font_size * 18.0f); 
		float max_width = std::max(550.0f, font_size * 32.0f); 
		
		float item_width = std::clamp(target_width, min_width, max_width);

		float button_height = font_size * 2.2f;
		ImVec2 button_size = ImVec2{ item_width, button_height };

		// 3. Hitung estimasi total tinggi seluruh elemen untuk centering vertikal
		float text_height = ImGui::GetTextLineHeight();
		float frame_height = ImGui::GetFrameHeight();
		float item_spacing = ImGui::GetStyle().ItemSpacing.y;

		float logoWidth = 120.0f;
		float logoHeight = 0.0f;
		if (renderer && renderer->m_icons != nullptr && renderer->m_icons_size.x > 0)
		{
			float aspectRatio = (float)renderer->m_icons_size.y / (float)renderer->m_icons_size.x;
			logoHeight = logoWidth * aspectRatio;
		}

		// Total tinggi = Logo + (3x Label Text) + (3x Input Box) + (1x Button) + Spacing
		float total_content_height = (text_height * 3.0f) + (frame_height * 3.0f) + button_size.y + (item_spacing * 8.0f);
		if (logoHeight > 0.0f)
		{
			total_content_height += logoHeight + (item_spacing * 2.0f);
		}

		// Hitung offset koordinat awal
		float center_x = (window_size.x - item_width) * 0.5f;
		float start_y = (window_size.y - total_content_height) * 0.5f;

		if (start_y > 10.0f)
		{
			ImGui::SetCursorPosY(start_y);
		}

		if (logoHeight > 0.0f)
		{
			ImGui::SetCursorPosX((window_size.x - logoWidth) * 0.5f);
			ImGui::Image((void*)renderer->m_icons, ImVec2(logoWidth, logoHeight));
			ImGui::Spacing();
			ImGui::Spacing();
		}

		// 4. Render Komponen Tampilan Login
		ImGui::BeginGroup();

		// --- Username Section ---
		ImGui::SetCursorPosX(center_x);
		ImGui::TextUnformatted("Username");

		ImGui::SetCursorPosX(center_x);
		ImGui::PushItemWidth(item_width);
		ImGui::InputText(xorstr("##Username"), g_user_authentication->username, IM_ARRAYSIZE(g_user_authentication->username));
		ImGui::PopItemWidth();

		ImGui::Spacing();

		// --- Password Section ---
		ImGui::SetCursorPosX(center_x);
		ImGui::TextUnformatted("Password");

		ImGui::SetCursorPosX(center_x);
		ImGui::PushItemWidth(item_width);
		ImGui::InputText(xorstr("##Password"), g_user_authentication->password, IM_ARRAYSIZE(g_user_authentication->password), ImGuiInputTextFlags_Password);
		ImGui::PopItemWidth();

		ImGui::Spacing();

		// --- Server Environment Section ---
		ImGui::SetCursorPosX(center_x);
		ImGui::TextUnformatted("Server Environment");

		ImGui::SetCursorPosX(center_x);
		ImGui::PushItemWidth(item_width);
		int current_env = static_cast<int>(environment_manager::get().get_current_environment());
		const char* env_items[] = {
			"Localhost (http://localhost:8180)",
			"Production (https://apie.rena.my.id)",
			"Custom"
		};
		if (ImGui::Combo(xorstr("##EnvironmentComboLogin"), &current_env, env_items, IM_ARRAYSIZE(env_items)))
		{
			environment_manager::get().set_environment(static_cast<Environment>(current_env));
		}
		ImGui::PopItemWidth();

		ImGui::Spacing();
		ImGui::Spacing();

		// --- Login Button ---
		ImGui::SetCursorPosX(center_x);
		if (ImGui::Button(xorstr("Login"), button_size))
		{
			g_thread_pool->add_job([] {
				if (g_user_authentication->login(g_user_authentication->username, g_user_authentication->password))
				{
					g_download_binary->generate_binaries();
					
					// Menggunakan 0 alih-alih NULL untuk clear buffer memory
					memset(g_user_authentication->password, 0, sizeof(g_user_authentication->password));

					LOG(HACKER) << g_user_authentication->get_message();
				}
			});
		}

		if (g_user_authentication && !g_user_authentication->get_message().empty())
		{
			ImGui::Spacing();
			ImGui::SetCursorPosX(center_x);
			ImVec4 msg_color = g_user_authentication->authorized() ? ImVec4(0.3f, 0.9f, 0.4f, 1.0f) : ImVec4(0.95f, 0.35f, 0.35f, 1.0f);
			ImGui::TextColored(msg_color, "%s", g_user_authentication->get_message().c_str());
		}

		ImGui::EndGroup();
	}
}