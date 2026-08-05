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

	// Wrapper Class UI Utama ImGui
	class ui
	{
	public:
		ui();
		~ui() = default;

		void init();
		void setup_dashboard_style();
		void render(class renderer* renderer_ptr);

		// ==========================================
		// WEB APP & DASHBOARD UI PRIMITIVE COMPONENTS
		// ==========================================
		static void card_begin(const char* id, const char* title, const char* subtitle = nullptr, float width = 0.0f, float height = 0.0f);
		static void card_end();

		static void stat_widget(const char* label, const char* value, const char* change_text, const char* icon_str, const ImVec4& accent_color);
		static void badge(const char* text, const ImVec4& bg_color, const ImVec4& text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		static void stepper_widget(const char* const steps[], int step_count, int current_step);
		
		static bool primary_button(const char* label, const ImVec2& size = ImVec2(0, 0), bool enabled = true);
		static bool secondary_button(const char* label, const ImVec2& size = ImVec2(0, 0), bool enabled = true);
		static bool input_text(const char* label, const char* hint, char* buf, size_t buf_size, bool is_password = false);

		// Notification Toasts
		void show_toast(const std::string& title, const std::string& message, ImVec4 color = ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
		void render_toasts();

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

		void fetch_data_from_server();

	private:
		void render_sidebar();
		void render_top_header();
		void render_content_area(class renderer* renderer_ptr);

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

	inline std::unique_ptr<ui> g_ui{};
}
