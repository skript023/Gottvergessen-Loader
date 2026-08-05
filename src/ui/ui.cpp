#include "ui/ui.hpp"
#include "ui/ui_views.hpp"
#include "renderer.hpp"
#include "api/user/user_authentication.hpp"
#include "api/remote/download_binary.hpp"
#include <imgui_internal.h>
#include <algorithm>

namespace gottvergessen
{
	ui::ui()
	{
		init();
	}

	void ui::init()
	{
		// Data will be loaded from server when user is authorized
		m_user_licenses.clear();
		m_data_loaded = false;
	}

	void ui::setup_dashboard_style()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		// Metric & Geometry Settings for Modern Web Dashboard Look
		style.WindowPadding = ImVec2(16.0f, 16.0f);
		style.FramePadding = ImVec2(12.0f, 8.0f);
		style.ItemSpacing = ImVec2(12.0f, 10.0f);
		style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
		style.IndentSpacing = 20.0f;
		style.ScrollbarSize = 10.0f;
		style.GrabMinSize = 10.0f;
		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.TabBorderSize = 0.0f;

		style.WindowRounding = 8.0f;
		style.ChildRounding = 6.0f;
		style.FrameRounding = 6.0f;
		style.PopupRounding = 6.0f;
		style.ScrollbarRounding = 5.0f;
		style.GrabRounding = 4.0f;
		style.TabRounding = 6.0f;

		style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);

		// Modern Dark Dashboard Slate Palette (Inspired by GitHub / Vercel Dark UI)
		ImVec4* colors = style.Colors;
		colors[ImGuiCol_Text]                  = ImVec4(0.94f, 0.96f, 0.98f, 1.00f);
		colors[ImGuiCol_TextDisabled]          = ImVec4(0.55f, 0.58f, 0.62f, 1.00f);
		colors[ImGuiCol_WindowBg]              = ImVec4(0.05f, 0.07f, 0.09f, 0.98f);
		colors[ImGuiCol_ChildBg]               = ImVec4(0.08f, 0.10f, 0.13f, 0.85f);
		colors[ImGuiCol_PopupBg]               = ImVec4(0.09f, 0.11f, 0.15f, 0.98f);
		colors[ImGuiCol_Border]                = ImVec4(0.19f, 0.22f, 0.26f, 0.60f);
		colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg]               = ImVec4(0.13f, 0.16f, 0.20f, 0.80f);
		colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.17f, 0.21f, 0.27f, 0.90f);
		colors[ImGuiCol_FrameBgActive]         = ImVec4(0.22f, 0.27f, 0.35f, 1.00f);
		colors[ImGuiCol_TitleBg]               = ImVec4(0.08f, 0.10f, 0.13f, 1.00f);
		colors[ImGuiCol_TitleBgActive]         = ImVec4(0.11f, 0.14f, 0.18f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.05f, 0.07f, 0.09f, 0.80f);
		colors[ImGuiCol_MenuBarBg]             = ImVec4(0.09f, 0.11f, 0.15f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.06f, 0.08f, 0.10f, 0.50f);
		colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.22f, 0.26f, 0.32f, 0.80f);
		colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.30f, 0.35f, 0.44f, 0.90f);
		colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.23f, 0.51f, 0.96f, 1.00f); // Electric Blue Accent
		colors[ImGuiCol_CheckMark]             = ImVec4(0.23f, 0.51f, 0.96f, 1.00f);
		colors[ImGuiCol_SliderGrab]            = ImVec4(0.23f, 0.51f, 0.96f, 0.90f);
		colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.35f, 0.60f, 0.98f, 1.00f);
		colors[ImGuiCol_Button]                = ImVec4(0.14f, 0.18f, 0.24f, 0.85f);
		colors[ImGuiCol_ButtonHovered]         = ImVec4(0.20f, 0.25f, 0.34f, 1.00f);
		colors[ImGuiCol_ButtonActive]          = ImVec4(0.23f, 0.51f, 0.96f, 1.00f);
		colors[ImGuiCol_Header]                = ImVec4(0.15f, 0.20f, 0.27f, 0.70f);
		colors[ImGuiCol_HeaderHovered]         = ImVec4(0.20f, 0.26f, 0.35f, 0.90f);
		colors[ImGuiCol_HeaderActive]          = ImVec4(0.23f, 0.51f, 0.96f, 1.00f);
		colors[ImGuiCol_Separator]             = ImVec4(0.19f, 0.22f, 0.26f, 0.60f);
		colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.23f, 0.51f, 0.96f, 0.80f);
		colors[ImGuiCol_SeparatorActive]       = ImVec4(0.23f, 0.51f, 0.96f, 1.00f);
		colors[ImGuiCol_ResizeGrip]            = ImVec4(0.19f, 0.22f, 0.26f, 0.50f);
		colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.23f, 0.51f, 0.96f, 0.80f);
		colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.23f, 0.51f, 0.96f, 1.00f);
		colors[ImGuiCol_Tab]                   = ImVec4(0.11f, 0.14f, 0.18f, 0.85f);
		colors[ImGuiCol_TabHovered]            = ImVec4(0.20f, 0.25f, 0.34f, 1.00f);
		colors[ImGuiCol_TabActive]             = ImVec4(0.16f, 0.21f, 0.28f, 1.00f);
		colors[ImGuiCol_PlotLines]             = ImVec4(0.23f, 0.51f, 0.96f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.96f, 0.35f, 0.23f, 1.00f);
		colors[ImGuiCol_PlotHistogram]         = ImVec4(0.23f, 0.51f, 0.96f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.16f, 0.72f, 0.53f, 1.00f);
		colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.23f, 0.51f, 0.96f, 0.35f);
		colors[ImGuiCol_DragDropTarget]        = ImVec4(0.16f, 0.72f, 0.53f, 0.90f);
		colors[ImGuiCol_NavHighlight]          = ImVec4(0.23f, 0.51f, 0.96f, 1.00f);
	}

	void ui::render(renderer* renderer_ptr)
	{
		setup_dashboard_style();

		// Fetch data from server once when user is authorized
		if (!m_data_loaded && g_user_authentication && g_user_authentication->authorized())
		{
			fetch_data_from_server();
		}

		// Main Layout Grid: Left Sidebar + Right Work Area (Header + Tab Content)
		ImGui::Columns(2, "DashboardMainGrid", false);

		// Set Sidebar fixed width (220px)
		ImGui::SetColumnWidth(0, 230.0f);

		// Render Sidebar Navigation
		render_sidebar();

		ImGui::NextColumn();

		// Render Top Header Bar & Work Area
		render_top_header();
		render_content_area(renderer_ptr);

		ImGui::Columns(1);

		// Toast Notifications Overlay
		render_toasts();
	}

	void ui::render_sidebar()
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.08f, 0.10f, 1.0f));
		ImGui::BeginChild("SidebarNavContainer", ImVec2(0, 0), true);
		{
			// Brand Logo & Title Header
			ImGui::Spacing();
			ImGui::SetCursorPosX(16.0f);
			ImGui::TextColored(ImVec4(0.23f, 0.51f, 0.96f, 1.0f), ICON_FA_SHIELD_ALT "  ELLOHIM");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "DASHBOARD");

			ImGui::SetCursorPosX(16.0f);
			ImGui::TextDisabled("Loader v1.0.4 Web Suite");

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Sidebar Navigation Menu Items
			auto render_nav_item = [this](NavTab tab, const char* label, const char* icon_str) {
				bool is_active = (m_active_tab == tab);
				if (is_active)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.51f, 0.96f, 0.25f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.23f, 0.51f, 0.96f, 0.35f));
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.70f, 1.00f, 1.0f));
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.18f, 0.24f, 0.6f));
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.84f, 0.88f, 1.0f));
				}

				std::string btn_label = std::string(icon_str) + "  " + label;
				if (ImGui::Button(btn_label.c_str(), ImVec2(-1, 38.0f)))
				{
					m_active_tab = tab;
				}

				ImGui::PopStyleColor(3);
				ImGui::Spacing();
			};

			render_nav_item(NavTab::BinaryDownload, "Binary & Launch", ICON_FA_ROCKET);
			render_nav_item(NavTab::MyLicenses,     "My Licenses",    ICON_FA_KEY);
			render_nav_item(NavTab::Settings,       "Settings",       ICON_FA_COG);

			// Bottom Profile / Session Card
			ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 75.0f);
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::SetCursorPosX(12.0f);
			std::string user_name = (g_user_authentication && !g_user_authentication->get_username().empty()) 
				? g_user_authentication->get_username() 
				: "Guest";
			std::string display_label = std::string(ICON_FA_USER) + "  " + user_name;
			ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", display_label.c_str());

			ImGui::SetCursorPosX(12.0f);
			std::string role_badge = (g_user_authentication && !g_user_authentication->get_role().empty())
				? g_user_authentication->get_role()
				: "VERIFIED CLIENT";
			ui::badge(role_badge.c_str(), ImVec4(0.16f, 0.72f, 0.53f, 0.25f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	void ui::render_top_header()
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.09f, 0.12f, 0.90f));
		ImGui::BeginChild("TopHeaderBarContainer", ImVec2(0, 50.0f), true, ImGuiWindowFlags_NoScrollbar);
		{
			// Title Breadcrumb
			const char* tab_titles[] = {
				"Binary Verification & Launch Engine",
				"User License Management",
				"Application Settings & Diagnostic Log"
			};
			int tab_idx = static_cast<int>(m_active_tab);

			ImGui::SetCursorPosY(12.0f);
			ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s", tab_titles[tab_idx]);

			// Right Header Status Badges & Logout Button
			ImGui::SameLine(ImGui::GetWindowWidth() - 380.0f);
			ImGui::SetCursorPosY(10.0f);

			ui::badge("API: ONLINE", ImVec4(0.10f, 0.40f, 0.25f, 0.6f), ImVec4(0.30f, 0.95f, 0.55f, 1.0f));
			ImGui::SameLine();
			ui::badge("LATENCY: 24ms", ImVec4(0.15f, 0.25f, 0.40f, 0.6f), ImVec4(0.40f, 0.75f, 1.0f, 1.0f));

			ImGui::SameLine(0, 15.0f);
			ImGui::SetCursorPosY(7.0f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.20f, 0.20f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.15f, 0.15f, 1.0f));
			
			std::string logout_btn_label = std::string(ICON_FA_SIGN_OUT_ALT) + " Logout";
			if (ImGui::Button(logout_btn_label.c_str(), ImVec2(80.0f, 32.0f)))
			{
				if (g_user_authentication)
				{
					g_user_authentication->logout();
				}
			}
			ImGui::PopStyleColor(3);
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	void ui::render_content_area(renderer* renderer_ptr)
	{
		ImGui::BeginChild("MainWorkAreaContainer", ImVec2(0, 0), false);
		{
			switch (m_active_tab)
			{
			case NavTab::BinaryDownload:
				ui_views::render_binary_download(this, renderer_ptr);
				break;
			case NavTab::MyLicenses:
				ui_views::render_my_licenses(this);
				break;
			case NavTab::Settings:
				ui_views::render_settings(this);
				break;
			}
		}
		ImGui::EndChild();
	}

	// =========================================================================
	// SERVER DATA FETCHING
	// =========================================================================

	void ui::fetch_data_from_server()
	{
		if (!g_user_authentication || !g_user_authentication->authorized())
			return;

		m_data_loaded = true;

		try
		{
			// ---- Fetch User Binaries from GET /binary/my-binaries ----
			if (g_download_binary)
			{
				g_download_binary->generate_binaries();
			}

			// ---- Fetch My Licenses from GET /license/my-licenses ----
			auto licenses_json = g_user_authentication->api_get(xorstr("http://localhost:8180/license/my-licenses"));
			if (!licenses_json.is_discarded() && licenses_json.contains("data"))
			{
				m_user_licenses.clear();
				auto& data = licenses_json["data"];
				if (data.is_array())
				{
					for (auto& l : data)
					{
						UserLicense lic;
						lic.license_id = l.value("id", "");

						// Extract product name from nested product object if included, or fallbacks
						if (l.contains("product") && l["product"].is_object() && l["product"].contains("name"))
						{
							lic.product_name = l["product"].value("name", "");
						}
						if (lic.product_name.empty())
						{
							lic.product_name = l.value("product_name", l.value("product_id", "Product License"));
						}

						lic.license_key = l.value("license_key", "");
						lic.issued_at = l.value("issued_at", l.value("created_at", ""));
						lic.expiry_date = l.value("expiry_date", "Lifetime");
						lic.status = l.value("status", "ACTIVE");
						m_user_licenses.push_back(lic);
					}
				}
			}

			LOG(INFO) << "Fetched " << m_user_licenses.size() << " licenses from server.";
		}
		catch (const std::exception& ex)
		{
			LOG(WARNING) << xorstr("Failed to fetch data from server: ") << ex.what();
		}
	}

	// =========================================================================
	// UI PRIMITIVE COMPONENT IMPLEMENTATIONS
	// =========================================================================

	void ui::card_begin(const char* id, const char* title, const char* subtitle, float width, float height)
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.11f, 0.15f, 0.90f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.19f, 0.22f, 0.27f, 0.60f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));

		ImGui::BeginChild(id, ImVec2(width, height), true, 0);
		{
			if (title)
			{
				ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s", title);
				if (subtitle)
				{
					ImGui::TextDisabled("%s", subtitle);
				}
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
			}
		}
	}

	void ui::card_end()
	{
		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);
	}

	void ui::stat_widget(const char* label, const char* value, const char* change_text, const char* icon_str, const ImVec4& accent_color)
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.11f, 0.15f, 0.90f));
		ImGui::BeginChild(label, ImVec2(0, 90.0f), true);
		{
			ImGui::SetCursorPos(ImVec2(14.0f, 12.0f));
			ImGui::TextDisabled("%s", label);

			ImGui::SetCursorPos(ImVec2(14.0f, 34.0f));
			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", value);
			ImGui::PopFont();

			if (change_text)
			{
				ImGui::SetCursorPos(ImVec2(14.0f, 62.0f));
				ImGui::TextColored(accent_color, "%s", change_text);
			}

			if (icon_str)
			{
				ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 40.0f, 14.0f));
				ImGui::TextColored(accent_color, "%s", icon_str);
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	void ui::badge(const char* text, const ImVec4& bg_color, const ImVec4& text_color)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, bg_color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg_color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, bg_color);
		ImGui::PushStyleColor(ImGuiCol_Text, text_color);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 2.0f));

		ImGui::Button(text);

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(4);
	}

	void ui::stepper_widget(const char* const steps[], int step_count, int current_step)
	{
		float total_width = ImGui::GetContentRegionAvail().x;
		float step_width = total_width / (float)step_count;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 p = ImGui::GetCursorScreenPos();

		for (int i = 0; i < step_count; i++)
		{
			bool is_done = i < current_step;
			bool is_current = i == current_step;

			ImVec4 circle_color = is_done ? ImVec4(0.16f, 0.72f, 0.53f, 1.0f) :
				(is_current ? ImVec4(0.23f, 0.51f, 0.96f, 1.0f) : ImVec4(0.25f, 0.28f, 0.33f, 1.0f));

			float center_x = p.x + (i * step_width) + (step_width * 0.5f);
			float center_y = p.y + 16.0f;

			// Draw connecting line between step nodes
			if (i < step_count - 1)
			{
				float next_center_x = p.x + ((i + 1) * step_width) + (step_width * 0.5f);
				ImU32 line_col = is_done ? IM_COL32(41, 184, 135, 255) : IM_COL32(60, 65, 75, 255);
				draw_list->AddLine(ImVec2(center_x, center_y), ImVec2(next_center_x, center_y), line_col, 3.0f);
			}

			// Draw node circle
			ImU32 circle_col = ImGui::ColorConvertFloat4ToU32(circle_color);
			draw_list->AddCircleFilled(ImVec2(center_x, center_y), 14.0f, circle_col);

			// Draw step number
			char num_str[4];
			snprintf(num_str, sizeof(num_str), "%d", i + 1);
			ImVec2 text_size = ImGui::CalcTextSize(num_str);
			draw_list->AddText(ImVec2(center_x - text_size.x * 0.5f, center_y - text_size.y * 0.5f), IM_COL32(255, 255, 255, 255), num_str);

			// Draw step title below
			ImVec2 label_size = ImGui::CalcTextSize(steps[i]);
			draw_list->AddText(ImVec2(center_x - label_size.x * 0.5f, center_y + 20.0f),
				is_current ? IM_COL32(240, 246, 252, 255) : IM_COL32(140, 148, 158, 255), steps[i]);
		}

		ImGui::Dummy(ImVec2(total_width, 55.0f));
	}

	bool ui::primary_button(const char* label, const ImVec2& size, bool enabled)
	{
		if (!enabled)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.18f, 0.22f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.55f, 0.60f, 1.0f));
			ImGui::Button(label, size);
			ImGui::PopStyleColor(2);
			return false;
		}

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.51f, 0.96f, 0.90f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.58f, 0.98f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.44f, 0.88f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

		bool pressed = ImGui::Button(label, size);

		ImGui::PopStyleColor(4);
		return pressed;
	}

	bool ui::secondary_button(const char* label, const ImVec2& size, bool enabled)
	{
		if (!enabled)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.15f, 0.18f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.45f, 0.50f, 1.0f));
			ImGui::Button(label, size);
			ImGui::PopStyleColor(2);
			return false;
		}

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.20f, 0.26f, 0.85f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.28f, 0.36f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.16f, 0.22f, 1.00f));

		bool pressed = ImGui::Button(label, size);

		ImGui::PopStyleColor(3);
		return pressed;
	}

	bool ui::input_text(const char* label, const char* hint, char* buf, size_t buf_size, bool is_password)
	{
		ImGuiInputTextFlags flags = is_password ? ImGuiInputTextFlags_Password : 0;
		return ImGui::InputTextWithHint(label, hint, buf, buf_size, flags);
	}

	void ui::show_toast(const std::string& title, const std::string& message, ImVec4 color)
	{
		m_toasts.push_back({ title, message, color, 4.0f, 0.0f });
	}

	void ui::render_toasts()
	{
		if (m_toasts.empty())
			return;

		float delta_time = ImGui::GetIO().DeltaTime;
		float toast_y = 20.0f;

		for (auto it = m_toasts.begin(); it != m_toasts.end(); )
		{
			it->timer += delta_time;
			if (it->timer >= it->duration)
			{
				it = m_toasts.erase(it);
				continue;
			}

			ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320.0f, toast_y), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(300.0f, 70.0f));
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

			char window_id[32];
			snprintf(window_id, sizeof(window_id), "##ToastWindow_%p", (void*)&(*it));

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.09f, 0.11f, 0.15f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_Border, it->color);
			ImGui::Begin(window_id, nullptr, flags);
			{
				ImGui::TextColored(it->color, "%s", it->title.c_str());
				ImGui::TextWrapped("%s", it->message.c_str());
			}
			ImGui::End();
			ImGui::PopStyleColor(2);

			toast_y += 80.0f;
			++it;
		}
	}
}
