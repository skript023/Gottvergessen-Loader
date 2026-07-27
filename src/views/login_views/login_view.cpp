#include "views/views.hpp"

#include "renderer.hpp"
#include "thread_pool.hpp"

#include <imgui.h>

#include "process/injection.hpp"
#include "api/remote/download_binary.hpp"
#include "api/user/user_authentication.hpp"
#include "api/costume_loader/costume_loader.hpp"

namespace gottvergessen
{
	void views::login_view(renderer* renderer)
	{
		ImVec2 window_size = ImGui::GetWindowSize();
		float font_size = ImGui::GetFontSize();

		// 2. Skala dinamis berbasis proporsi window & skala font
		// Gunakan persentase window (misal 45%) tetapi dinaikkan batas minimum & maksimumnya
		// agar terlihat proporsional di layar 1080p/2K/4K.
		float target_width = window_size.x * 0.45f;
		
		// Batas dinamis berbasis font size (18em - 32em) agar adaptif terhadap DPI/Font scale
		float min_width = std::max(260.0f, font_size * 18.0f); 
		float max_width = std::max(550.0f, font_size * 32.0f); 
		
		float item_width = std::clamp(target_width, min_width, max_width);

		// Tinggi tombol & frame dibuat proporsional terhadap ukuran font (misal 2.2x font size)
		float button_height = font_size * 2.2f;
		ImVec2 button_size = ImVec2{ item_width, button_height };

		// 3. Hitung estimasi total tinggi seluruh elemen untuk centering vertikal
		float text_height = ImGui::GetTextLineHeight();
		float frame_height = ImGui::GetFrameHeight();
		float item_spacing = ImGui::GetStyle().ItemSpacing.y;

		// Total tinggi = (2x Label Text) + (2x Input Box) + (1x Button) + Spacing
		float total_content_height = (text_height * 2.0f) + (frame_height * 2.0f) + button_size.y + (item_spacing * 5.0f);

		// Hitung offset koordinat awal
		float center_x = (window_size.x - item_width) * 0.5f;
		float start_y = (window_size.y - total_content_height) * 0.5f;

		// Terapkan posisi Y awal jika masih berada dalam batasan window
		if (start_y > 10.0f)
		{
			ImGui::SetCursorPosY(start_y);
		}

		// 4. Render Komponen Tampilan Login
		ImGui::BeginGroup();

		if (renderer && renderer->m_icons != nullptr)
		{
			float logoWidth = 40.0f; // Ukuran diperkecil sedikit agar pas di dalam card/konten
			float aspectRatio = (float)renderer->m_icons_size.y / (float)renderer->m_icons_size.x;
			float logoHeight = logoWidth * aspectRatio;

			float windowWidth = ImGui::GetWindowSize().x;
			ImGui::SetCursorPosX((windowWidth - logoWidth) * 0.5f);
			ImGui::Image((void*)renderer->m_icons, ImVec2(logoWidth, logoHeight));
			ImGui::Spacing();
		}

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
		ImGui::Spacing();

		// --- Login Button ---
		ImGui::SetCursorPosX(center_x);
		if (ImGui::Button(xorstr("Login"), button_size))
		{
			g_thread_pool->add_job([] {
				if (g_user_authentication->login(g_user_authentication->username, g_user_authentication->password))
				{
					g_costume_loader->execute();
					g_download_binary->generate_binaries();
					
					// Menggunakan 0 alih-alih NULL untuk clear buffer memory
					memset(g_user_authentication->password, 0, sizeof(g_user_authentication->password));

					LOG(HACKER) << g_user_authentication->get_message();
				}
			});
		}

		ImGui::EndGroup();
	}
}