#include "ui/ui.hpp"
#include "ui/ui_views.hpp"
#include "renderer.hpp"
#include "api/user/user_authentication.hpp"
#include "api/remote/download_binary.hpp"
#include "api/environment.hpp"
#include <imgui_internal.h>
#include <algorithm>

namespace gottvergessen
{
	ui::ui()
	{
		init_impl();
	}

	void ui::init_impl()
	{
		// Data will be loaded from server when user is authorized
		m_user_licenses.clear();
		m_data_loaded = false;
	}

	void ui::setup_dashboard_style_impl()
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

	void ui::render_impl(renderer* renderer_ptr)
	{
		setup_dashboard_style_impl();

		// Fetch data from server once when user is authorized
		if (!m_data_loaded && user_authentication::authorized())
		{
			fetch_data_from_server_impl();
		}

		float sidebar_width = 220.0f;
		
		// Left Sidebar Container
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.08f, 0.10f, 1.0f));
		ImGui::BeginChild("SidebarNavContainer", ImVec2(sidebar_width, 0), true);
		render_sidebar_impl();
		ImGui::EndChild();
		ImGui::PopStyleColor();

		ImGui::SameLine(0.0f, 10.0f);

		// Right Main Work Area Container (Header + Tab Content)
		ImGui::BeginChild("WorkAreaContainer", ImVec2(0, 0), false);
		render_top_header_impl();
		render_content_area_impl(renderer_ptr);
		ImGui::EndChild();

		// Toast Notifications Overlay
		render_toasts_impl();
	}

	void ui::render_sidebar_impl()
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
		float max_y = ImGui::GetWindowHeight();
		float footer_height = 80.0f;
		if (max_y > footer_height + 50.0f)
		{
			ImGui::SetCursorPosY(max_y - footer_height);
		}
		ImGui::Separator();
		ImGui::Spacing();

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 p = ImGui::GetCursorScreenPos();
		
		std::string user_name = (!user_authentication::get().get_username().empty()) 
			? user_authentication::get().get_username() 
			: "Guest";

		// Avatar circle parameters
		float avatar_size = 38.0f;
		ImVec2 avatar_center = ImVec2(p.x + 16.0f + avatar_size * 0.5f, p.y + 6.0f + avatar_size * 0.5f);

		ID3D11ShaderResourceView* avatar_tex = user_authentication::get_avatar_texture();
		if (avatar_tex)
		{
			ImVec2 p_min = ImVec2(avatar_center.x - avatar_size * 0.5f, avatar_center.y - avatar_size * 0.5f);
			ImVec2 p_max = ImVec2(avatar_center.x + avatar_size * 0.5f, avatar_center.y + avatar_size * 0.5f);
			draw_list->AddImageRounded((ImTextureID)avatar_tex, p_min, p_max, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE, avatar_size * 0.5f);
			draw_list->AddCircle(avatar_center, avatar_size * 0.5f, IM_COL32(60, 130, 245, 255), 0, 1.8f);
		}
		else
		{
			// Outer glowing circle & background for avatar fallback
			draw_list->AddCircleFilled(avatar_center, avatar_size * 0.5f, IM_COL32(28, 45, 75, 240));
			draw_list->AddCircle(avatar_center, avatar_size * 0.5f, IM_COL32(60, 130, 245, 255), 0, 1.8f);

			// Initial letter from username
			char initial = user_name.empty() ? 'G' : (char)toupper(user_name[0]);
			char init_str[2] = { initial, '\0' };
			ImVec2 text_sz = ImGui::CalcTextSize(init_str);
			draw_list->AddText(ImVec2(avatar_center.x - text_sz.x * 0.5f, avatar_center.y - text_sz.y * 0.5f), IM_COL32(230, 240, 255, 255), init_str);
		}

		// Status online dot on bottom right of avatar
		ImVec2 status_dot = ImVec2(avatar_center.x + avatar_size * 0.33f, avatar_center.y + avatar_size * 0.33f);
		draw_list->AddCircleFilled(status_dot, 5.0f, IM_COL32(40, 220, 130, 255));
		draw_list->AddCircle(status_dot, 5.0f, IM_COL32(15, 25, 35, 255), 0, 1.5f);

		// Text layout next to Avatar
		float text_offset_x = 16.0f + avatar_size + 10.0f;
		ImGui::SetCursorScreenPos(ImVec2(p.x + text_offset_x, p.y + 6.0f));
		ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s", user_name.c_str());

		ImGui::SetCursorScreenPos(ImVec2(p.x + text_offset_x, p.y + 24.0f));
		std::string role_badge = (!user_authentication::get().get_role().empty())
			? user_authentication::get().get_role()
			: "VERIFIED CLIENT";
		badge_impl(role_badge.c_str(), ImVec4(0.16f, 0.72f, 0.53f, 0.25f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));
	}

	void ui::render_top_header_impl()
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.09f, 0.12f, 0.90f));
		ImGui::BeginChild("TopHeaderBarContainer", ImVec2(0, 46.0f), true, ImGuiWindowFlags_NoScrollbar);
		{
			float avail_w = ImGui::GetContentRegionAvail().x;
			float right_width = 335.0f; // total width of right badges + logout button

			// Title Breadcrumb
			const char* tab_titles[] = {
				"Binary Verification & Launch Engine",
				"User License Management",
				"Application Settings & Diagnostic Log"
			};
			int tab_idx = static_cast<int>(m_active_tab);

			ImGui::SetCursorPosY(10.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s", tab_titles[tab_idx]);

			// Right Header Status Badges & Logout Button
			if (avail_w > right_width + 150.0f)
			{
				ImGui::SameLine(avail_w - right_width);
			}
			else
			{
				ImGui::SameLine();
			}

			badge_impl("API: ONLINE", ImVec4(0.10f, 0.40f, 0.25f, 0.6f), ImVec4(0.30f, 0.95f, 0.55f, 1.0f));
			ImGui::SameLine(0, 8.0f);
			badge_impl("LATENCY: 24ms", ImVec4(0.15f, 0.25f, 0.40f, 0.6f), ImVec4(0.40f, 0.75f, 1.0f, 1.0f));
			ImGui::SameLine(0, 10.0f);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.20f, 0.20f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.15f, 0.15f, 1.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 4.0f));
			
			std::string logout_btn_label = std::string(ICON_FA_SIGN_OUT_ALT) + " Logout";
			if (ImGui::Button(logout_btn_label.c_str()))
			{
				user_authentication::logout();
			}
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(3);
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	void ui::render_content_area_impl(renderer* renderer_ptr)
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
	// SERVER DATA FETCHING IMPLEMENTATION
	// =========================================================================

	void ui::fetch_data_from_server_impl()
	{
		if (!user_authentication::authorized())
			return;

		m_data_loaded = true;

		try
		{
			// ---- Fetch User Binaries from GET /binary/my-binaries ----
			download_binary::generate_binaries();

			// ---- Fetch My Licenses from GET /license/my-licenses ----
			auto licenses_json = user_authentication::get().api_get(environment_manager::get().get_url("/license/my-licenses"));
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

	void ui::card_begin_impl(const char* id, const char* title, const char* subtitle, float width, float height)
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

	void ui::card_end_impl()
	{
		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);
	}

	void ui::stat_widget_impl(const char* label, const char* value, const char* change_text, const char* icon_str, const ImVec4& accent_color)
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

	void ui::badge_impl(const char* text, const ImVec4& bg_color, const ImVec4& text_color)
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

	void ui::stepper_widget_impl(const char* const steps[], int step_count, int current_step)
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

	bool ui::primary_button_impl(const char* label, const ImVec2& size, bool enabled)
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

	bool ui::secondary_button_impl(const char* label, const ImVec2& size, bool enabled)
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

	bool ui::input_text_impl(const char* label, const char* hint, char* buf, size_t buf_size, bool is_password)
	{
		ImGuiInputTextFlags flags = is_password ? ImGuiInputTextFlags_Password : 0;
		return ImGui::InputTextWithHint(label, hint, buf, buf_size, flags);
	}

	void ui::show_toast_impl(const std::string& title, const std::string& message, ImVec4 color)
	{
		m_toasts.push_back({ title, message, color, 4.0f, 0.0f });
	}

	void ui::render_toasts_impl()
	{
		if (m_toasts.empty())
			return;

		float delta_time = ImGui::GetIO().DeltaTime;
		float display_x = ImGui::GetIO().DisplaySize.x;
		float display_y = ImGui::GetIO().DisplaySize.y;

		int index = 0;
		for (auto it = m_toasts.begin(); it != m_toasts.end(); )
		{
			it->timer += delta_time;
			if (it->timer >= it->duration)
			{
				it = m_toasts.erase(it);
				continue;
			}

			float toast_y = display_y - 85.0f - (index * 78.0f);

			ImGui::SetNextWindowPos(ImVec2(display_x - 320.0f, toast_y), ImGuiCond_Always);
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

			index++;
			++it;
		}
	}
}
