#include "gui.hpp"
#include "api/http_request.hpp"
#include "renderer.hpp"
#include "thread_pool.hpp"
#include "views/views.hpp"


#include "api/user/user_authentication.hpp"
#include "ui/ui.hpp"

#include <imgui.h>

namespace gottvergessen
{
	void gui::dx_init()
	{
		ui::setup_dashboard_style();

		if (g_user_authentication)
		{
			g_user_authentication->check_auto_login();
		}
	}

	void gui::dx_on_tick(renderer* renderer)
	{
		HWND hwnd = (renderer && renderer->m_hwnd) ? renderer->m_hwnd : (g_renderer ? g_renderer->m_hwnd : NULL);

		// Kunci ImGui selalu di (0,0) internal HWND dan ukurannya selalu menyamai
		// HWND Client Area
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

		// Window Flags dikunci agar posisi dan ukuran ImGui tidak terlepas dari HWND
		ImGuiWindowFlags window_flags =
		    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		ImGui::Begin("DesktopMainWindow", &m_opened, window_flags);
		ImGui::PopStyleVar(3);

		ImVec2 current_size = ImGui::GetWindowSize();
		if (hwnd && ((int)current_size.x != (int)io.DisplaySize.x || (int)current_size.y != (int)io.DisplaySize.y))
		{
			::SetWindowPos(hwnd, NULL, 0, 0, (int)current_size.x, (int)current_size.y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		// =========================================================================
		// 1. TAMPILAN CUSTOM TITLE BAR
		// =========================================================================
		float titlebar_height = 36.0f;
		ImVec2 titlebar_size(ImGui::GetWindowWidth(), titlebar_height);

		// Frame background untuk titlebar
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.09f, 0.11f, 1.0f));
		ImGui::BeginChild("CustomTitleBar", titlebar_size, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		{
			float ibutton_width = 44.0f;
			float drag_width = ImGui::GetWindowWidth() - (ibutton_width * 3);
			ImGui::SetCursorPos(ImVec2(0, 0));
			ImGui::InvisibleButton("##titlebar_drag_zone",
			    ImVec2(drag_width, titlebar_height));
			if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				if (hwnd)
				{
					::ReleaseCapture();
					::SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);

					io.MouseDown[0] = false;
					io.MouseClicked[0] = false;
				}
			}

			// App Icon & Title Visual
			ImGui::SetCursorPos(
			    ImVec2(12, (titlebar_height - 20.0f) * 0.5f)); // Centered vertically in 36px height

			if (renderer && renderer->m_icons != nullptr)
			{
				// Logo dikecilkan agar pas di title bar (tinggi misal 20px)
				float logoHeight = 20.0f;
				float aspectRatio =
				    (float)renderer->m_icons_size.x / (float)renderer->m_icons_size.y;
				float logoWidth = logoHeight * aspectRatio;

				ImGui::Image((void*)renderer->m_icons, ImVec2(logoWidth, logoHeight));
			}
			else
			{
				ImGui::TextColored(ImVec4(0.40f, 0.60f, 1.0f, 1.0f), "[G]");
			}

			ImGui::SameLine(0, 8);
			ImGui::SetCursorPosY((titlebar_height - ImGui::GetTextLineHeight()) * 0.5f);
			ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f),
			    "Gottvergessen Dashboard Loader");

			// Status Badge / Version Tag Visual
			ImGui::SameLine(0, 10);
			ImGui::SetCursorPosY((titlebar_height - (ImGui::GetTextLineHeight() + 4.0f)) * 0.5f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 2));
			ImGui::Button("v1.0.4 - ONLINE");
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor();

			// Visual Window Controls di Kanan Atas (Minimize, Maximize, Close)
			float button_width = 44.0f;
			ImGui::SetCursorPos(
			    ImVec2(ImGui::GetWindowWidth() - (button_width * 3), 0));

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
			    ImVec4(0.20f, 0.22f, 0.27f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,
			    ImVec4(0.15f, 0.17f, 0.20f, 1.0f));

			// Tombol MINIMIZE
			if (ImGui::Button(ICON_FA_WINDOW_MINIMIZE,
			        ImVec2(button_width, titlebar_height)))
			{
				if (hwnd)
					ShowWindow(hwnd, SW_MINIMIZE);
			}

			ImGui::SameLine(0, 0);

			// Tombol MAXIMIZE / RESTORE
			const char* max_icon = (hwnd && IsZoomed(hwnd)) ? ICON_FA_WINDOW_RESTORE : ICON_FA_WINDOW_MAXIMIZE;
			if (ImGui::Button(max_icon, ImVec2(button_width, titlebar_height)))
			{
				if (hwnd)
				{
					if (IsZoomed(hwnd))
						ShowWindow(hwnd, SW_RESTORE);
					else
						ShowWindow(hwnd, SW_MAXIMIZE);
				}
			}

			ImGui::SameLine(0, 0);

			// Tombol CLOSE
			ImGui::PopStyleColor(2);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
			    ImVec4(0.85f, 0.20f, 0.20f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,
			    ImVec4(0.65f, 0.10f, 0.10f, 1.0f));

			if (ImGui::Button("X", ImVec2(button_width, titlebar_height)))
			{
				m_opened = false;
			}

			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar();
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();

		// =========================================================================
		// 2. TAMPILAN KONTEN UTAMA (DASHBOARD JIKA LOGGED IN, LOGIN VIEW JIKA BELUM)
		// =========================================================================
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.07f, 0.09f, 0.95f));

		ImGui::BeginChild("MainContentContainer", ImVec2(0, 0), false);
		{
			if (g_user_authentication && g_user_authentication->authorized())
			{
				ui::render(renderer);
			}
			else
			{
				views::login_view(renderer);
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::End();
	}
} // namespace gottvergessen
