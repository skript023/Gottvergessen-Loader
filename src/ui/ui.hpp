#pragma once

#include "common.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <memory>

namespace gottvergessen
{
	// Enum Navigasi Tab Loader UI
	enum class NavTab
	{
		BinaryDownload = 0,
		MyLicenses,
		Settings
	};

	// Model Data Lisensi Pengguna
	struct UserLicense
	{
		std::string license_id;
		std::string product_name;
		std::string license_key;
		std::string issued_at;
		std::string expiry_date;
		std::string status = "ACTIVE";
	};

	// Wrapper Class UI Utama ImGui (Singleton dengan Static Facade)
	class ui
	{
	public:
		static ui& instance()
		{
			static ui instance_val;
			return instance_val;
		}

		static ui& get()
		{
			return instance();
		}

		// Static Facade API -> Delegasi ke method private *_impl()
		static void init() { instance().init_impl(); }
		static void setup_dashboard_style() { instance().setup_dashboard_style_impl(); }
		static void render(class renderer* renderer_ptr) { instance().render_impl(renderer_ptr); }

		// Web App & Dashboard UI Primitive Components
		static void card_begin(const char* id, const char* title, const char* subtitle = nullptr, float width = 0.0f, float height = 0.0f)
		{
			instance().card_begin_impl(id, title, subtitle, width, height);
		}
		static void card_end() { instance().card_end_impl(); }

		static void stat_widget(const char* label, const char* value, const char* change_text, const char* icon_str, const ImVec4& accent_color)
		{
			instance().stat_widget_impl(label, value, change_text, icon_str, accent_color);
		}
		static void badge(const char* text, const ImVec4& bg_color, const ImVec4& text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
		{
			instance().badge_impl(text, bg_color, text_color);
		}
		static void stepper_widget(const char* const steps[], int step_count, int current_step)
		{
			instance().stepper_widget_impl(steps, step_count, current_step);
		}

		static bool primary_button(const char* label, const ImVec2& size = ImVec2(0, 0), bool enabled = true)
		{
			return instance().primary_button_impl(label, size, enabled);
		}
		static bool secondary_button(const char* label, const ImVec2& size = ImVec2(0, 0), bool enabled = true)
		{
			return instance().secondary_button_impl(label, size, enabled);
		}
		static bool input_text(const char* label, const char* hint, char* buf, size_t buf_size, bool is_password = false)
		{
			return instance().input_text_impl(label, hint, buf, buf_size, is_password);
		}

		// Notification Toasts
		static void show_toast(const std::string& title, const std::string& message, ImVec4 color = ImVec4(0.2f, 0.7f, 1.0f, 1.0f))
		{
			instance().show_toast_impl(title, message, color);
		}
		static void render_toasts() { instance().render_toasts_impl(); }

		static void fetch_data_from_server() { instance().fetch_data_from_server_impl(); }

	public:
		// Navigation State
		NavTab m_active_tab = NavTab::BinaryDownload;

		// User Licenses
		std::vector<UserLicense> m_user_licenses;

		// Data loading state
		bool m_data_loaded = false;

		// Binary Download & Injection State
		std::string m_remote_version = "v1.0.4";
		std::string m_local_version = "v1.0.0";
		bool m_has_update = true;
		bool m_is_downloading = false;
		float m_download_progress = 0.0f;
		bool m_binary_downloaded = false;
		bool m_injected = false;

	private:
		ui();
		~ui() = default;

		ui(const ui&) = delete;
		ui& operator=(const ui&) = delete;

		// Implementation Private Methods (*_impl suffix)
		void init_impl();
		void setup_dashboard_style_impl();
		void render_impl(class renderer* renderer_ptr);

		void card_begin_impl(const char* id, const char* title, const char* subtitle, float width, float height);
		void card_end_impl();

		void stat_widget_impl(const char* label, const char* value, const char* change_text, const char* icon_str, const ImVec4& accent_color);
		void badge_impl(const char* text, const ImVec4& bg_color, const ImVec4& text_color);
		void stepper_widget_impl(const char* const steps[], int step_count, int current_step);

		bool primary_button_impl(const char* label, const ImVec2& size, bool enabled);
		bool secondary_button_impl(const char* label, const ImVec2& size, bool enabled);
		bool input_text_impl(const char* label, const char* hint, char* buf, size_t buf_size, bool is_password);

		void show_toast_impl(const std::string& title, const std::string& message, ImVec4 color);
		void render_toasts_impl();

		void fetch_data_from_server_impl();

		void render_sidebar_impl();
		void render_top_header_impl();
		void render_content_area_impl(class renderer* renderer_ptr);

		// Dynamic Toast Notification structure
		struct ToastNotification
		{
			std::string title;
			std::string message;
			ImVec4 color;
			float duration = 4.0f;
			float timer = 0.0f;
		};
		std::vector<ToastNotification> m_toasts;
	};
}
