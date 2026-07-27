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
		ImVec2 screen_res{ 0, 0 };
		ImVec2 window_pos{ 0, 0 };
		ImVec2 window_size{ 600, 400 };
		if (m_init_pos == false)
		{
			RECT screen_rect;
			GetWindowRect(GetDesktopWindow(), &screen_rect);
			screen_res = ImVec2(float(screen_rect.right), float(screen_rect.bottom));
			window_pos.x = (screen_res.x - window_size.x) * 0.5f;
			window_pos.y = (screen_res.y - window_size.y) * 0.5f;
			m_init_pos = true;
		}

		ImGui::SetNextWindowPos(ImVec2(window_pos.x, window_pos.y), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(window_size.x, window_size.y), ImGuiCond_FirstUseEver);

		// Window Flags tanpa border bawaan ImGui agar tampilan murni seperti custom desktop app
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar 
									| ImGuiWindowFlags_NoCollapse 
									| ImGuiWindowFlags_NoSavedSettings 
									| ImGuiWindowFlags_NoBringToFrontOnFocus
									| ImGuiWindowFlags_MenuBar;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		
		ImGui::Begin("DesktopMainWindow", &m_opened, window_flags);
		ImGui::PopStyleVar(3);

		// =========================================================================
		// 1. TAMPILAN CUSTOM TITLE BAR (DESAIN DESKTOP MODERN)
		// =========================================================================
		float titlebar_height = 36.0f;
		ImVec2 titlebar_size(ImGui::GetWindowWidth(), titlebar_height);

		// Frame background untuk titlebar (Sleek Dark Theme)
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.09f, 0.11f, 1.0f));
		ImGui::BeginChild("CustomTitleBar", titlebar_size, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		{
			// App Icon & Title Visual
			ImGui::SetCursorPos(ImVec2(12, 8));
			ImGui::TextColored(ImVec4(0.40f, 0.60f, 1.0f, 1.0f), "[G]"); // Accent Logo
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
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // Flat transparan
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.22f, 0.27f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.17f, 0.20f, 1.0f));

			// Tampilan Tombol MINIMIZE (-)
			if (ImGui::Button(ICON_FA_WINDOW_MINIMIZE, ImVec2(button_width, titlebar_height)))
			{
				// Tampilan visual saja
			}

			ImGui::SameLine(0, 0);

			// Tampilan Tombol MAXIMIZE ([ ])
			if (ImGui::Button(ICON_FA_WINDOW_MAXIMIZE, ImVec2(button_width, titlebar_height)))
			{
				// Tampilan visual saja
			}

			ImGui::SameLine(0, 0);

			// Tampilan Tombol CLOSE (X) - Merah saat Hover
			ImGui::PopStyleColor(2); // Pop Hovered & Active
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.20f, 0.20f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.10f, 0.10f, 1.0f));

			if (ImGui::Button("X", ImVec2(button_width, titlebar_height)))
			{
				m_opened = false;
			}

			ImGui::PopStyleColor(3); // Pop Button, Hovered, Active
			ImGui::PopStyleVar();    // Pop FrameRounding
		}
		ImGui::EndChild();
		ImGui::PopStyleColor(); // Pop Titlebar Background

		// =========================================================================
		// 2. TAMPILAN NAVIGATION & MENU BAR
		// =========================================================================
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.12f, 0.12f, 0.15f, 1.0f));
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit", "Alt+F4"))
				{
					m_opened = false;
					PostQuitMessage(0);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Theme"))
			{
				ImGui::MenuItem("Dark Theme (Active)");
				ImGui::MenuItem("Cyberpunk Theme");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				ImGui::MenuItem("Documentation");
				ImGui::MenuItem("About");
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		// =========================================================================
		// 3. TAMPILAN KONTEN UTAMA (DESAIN LAYOUT MODERN)
		// =========================================================================
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.09f, 0.50f));
		
		ImGui::BeginChild("MainContentContainer", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
		{
			// Visual Header Dashboard / Loader Title
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "WELCOME TO GOTTVERGESSEN");
			ImGui::TextDisabled("Select an option below to proceed with authentication or configuration.");
			ImGui::Separator();
			ImGui::Spacing();

			if (!g_user_authentication->authorized())
			{
				views::login_view();
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