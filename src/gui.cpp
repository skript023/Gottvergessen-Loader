#include "gui.hpp"
#include "renderer.hpp"
#include "thread_pool.hpp"
#include "views/views.hpp"
#include "api/http_request.hpp"

#include "api/user/user_authentication.hpp"

#include <imgui.h>

namespace gottvergessen
{
	void gui::dx_init()
	{
		auto &style = ImGui::GetStyle();
		style.WindowPadding = { 10.f, 10.f };
		style.PopupRounding = 0.f;
		style.FramePadding = { 8.f, 4.f };
		style.ItemSpacing = { 10.f, 8.f };
		style.ItemInnerSpacing = { 6.f, 6.f };
		style.TouchExtraPadding = { 0.f, 0.f };
		style.IndentSpacing = 21.f;
		style.ScrollbarSize = 15.f;
		style.GrabMinSize = 8.f;
		style.WindowBorderSize = 1.f;
		style.ChildBorderSize = 0.f;
		style.PopupBorderSize = 1.f;
		style.FrameBorderSize = 0.f;
		style.TabBorderSize = 0.f;
		style.WindowRounding = 0.f;
		style.ChildRounding = 0.f;
		style.FrameRounding = 0.f;
		style.ScrollbarRounding = 0.f;
		style.GrabRounding = 0.f;
		style.TabRounding = 0.f;
		style.WindowTitleAlign = { 0.5f, 0.5f };
		style.ButtonTextAlign = { 0.5f, 0.5f };
		style.DisplaySafeAreaPadding = { 3.f, 3.f };

		auto &colors = style.Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(1.00f, 0.90f, 0.19f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.30f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.21f, 0.21f, 0.21f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.21f, 0.21f, 0.21f, 0.78f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.27f, 0.27f, 0.54f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.39f, 0.38f, 0.38f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.41f, 0.41f, 0.41f, 0.74f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.41f, 0.41f, 0.78f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.41f, 0.41f, 0.41f, 0.87f);
		colors[ImGuiCol_Header] = ImVec4(0.37f, 0.37f, 0.37f, 0.31f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.38f, 0.38f, 0.38f, 0.37f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.37f, 0.37f, 0.37f, 0.51f);
		colors[ImGuiCol_Separator] = ImVec4(0.38f, 0.38f, 0.38f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.46f, 0.46f, 0.46f, 0.50f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.64f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.21f, 0.21f, 0.21f, 0.86f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.27f, 0.27f, 0.27f, 0.86f);
		colors[ImGuiCol_TabActive] = ImVec4(0.34f, 0.34f, 0.34f, 0.86f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.10f, 0.97f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}

	void gui::dx_on_tick(renderer* renderer)
	{
		HWND hwnd = (renderer && renderer->m_hwnd) ? renderer->m_hwnd : (g_renderer ? g_renderer->m_hwnd : NULL);

        // Kunci ImGui selalu di (0,0) internal HWND dan ukurannya selalu menyamai HWND Client Area
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

        // Window Flags dikunci agar posisi dan ukuran ImGui tidak terlepas dari HWND
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar 
                                      | ImGuiWindowFlags_NoCollapse 
                                      | ImGuiWindowFlags_NoSavedSettings 
                                      | ImGuiWindowFlags_NoMove
                                      | ImGuiWindowFlags_NoBringToFrontOnFocus;

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
            ImGui::InvisibleButton("##titlebar_drag_zone", ImVec2(drag_width, titlebar_height));
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
            ImGui::SetCursorPos(ImVec2(12, (titlebar_height - 20.0f) * 0.5f)); // Centered vertically in 36px height

            if (renderer && renderer->m_icons != nullptr)
            {
                // Logo dikecilkan agar pas di title bar (tinggi misal 20px)
                float logoHeight = 20.0f;
                float aspectRatio = (float)renderer->m_icons_size.x / (float)renderer->m_icons_size.y;
                float logoWidth = logoHeight * aspectRatio;

                ImGui::Image((void*)renderer->m_icons, ImVec2(logoWidth, logoHeight));
            }
            else
            {
                ImGui::TextColored(ImVec4(0.40f, 0.60f, 1.0f, 1.0f), "[G]");
            }

            ImGui::SameLine(0, 8);
            ImGui::SetCursorPosY(8);
            ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "Gottvergessen Sense Loader");

            // Status Badge / Version Tag Visual
            ImGui::SameLine(0, 10);
            ImGui::SetCursorPosY(7);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 2));
            ImGui::Button("v1.0.4 - ONLINE");
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();

            // Visual Window Controls di Kanan Atas (Minimize, Maximize, Close)
            float button_width = 44.0f;
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - (button_width * 3), 0));

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.22f, 0.27f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.17f, 0.20f, 1.0f));

            // Tombol MINIMIZE
            if (ImGui::Button(ICON_FA_WINDOW_MINIMIZE, ImVec2(button_width, titlebar_height)))
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
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.20f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.10f, 0.10f, 1.0f));

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
        // 2. TAMPILAN KONTEN UTAMA
        // =========================================================================
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.09f, 0.50f));
        
        ImGui::BeginChild("MainContentContainer", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
        {
            auto TextCentered = [](const char* text, const ImVec4& color = ImVec4(1, 1, 1, 1), bool disabled = false) {
				float windowWidth = ImGui::GetWindowSize().x;
				float textWidth   = ImGui::CalcTextSize(text).x;

				// Geser kursor ke posisi tengah
				ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

				if (disabled) {
					ImGui::TextDisabled("%s", text);
				} else {
					ImGui::TextColored(color, "%s", text);
				}
			};

			// Render Teks Rata Tengah
			TextCentered("WELCOME TO GOTTVERGESSEN");
			TextCentered("Select an option below to proceed with authentication or configuration.", ImVec4(), true);
			
            ImGui::Separator();
            ImGui::Spacing();

            if (!g_user_authentication->authorized())
            {
                views::login_view(renderer);
            }
            else
            {
                views::injection_view();
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        ImGui::End();
	}
}