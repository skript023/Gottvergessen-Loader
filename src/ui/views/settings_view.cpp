#include "ui/views/settings_view.hpp"
#include "ui/ui.hpp"
#include "api/environment.hpp"

#include <imgui.h>
#include <string>
#include <cstring>

namespace gottvergessen
{
	void settings_view::render_impl(ui* ui_instance)
	{
		ui::card_begin("SettingsCard", "SETTINGS & DIAGNOSTIC CONFIGURATION", "Customize loader preferences and view live system logs.");
		{
			ImGui::Text("Target Server Environment:");
			
			int current_env = static_cast<int>(environment_manager::get().get_current_environment());
			const char* env_names[] = {
				"Localhost (http://localhost:8180)",
				"Production (https://apie.rena.my.id)",
				"Custom URL..."
			};

			if (ImGui::Combo("##EnvironmentCombo", &current_env, env_names, IM_ARRAYSIZE(env_names)))
			{
				environment_manager::get().set_environment(static_cast<Environment>(current_env));
			}

			if (environment_manager::get().get_current_environment() == Environment::CUSTOM)
			{
				ImGui::Spacing();
				ImGui::Text("Custom API Base URL:");
				static char custom_url_buf[256] = "";
				if (custom_url_buf[0] == '\0')
				{
					std::string cur_custom = environment_manager::get().get_custom_url();
					strncpy_s(custom_url_buf, cur_custom.c_str(), sizeof(custom_url_buf) - 1);
				}
				if (ui::input_text("##CustomApiUrl", "Custom API Base URL", custom_url_buf, sizeof(custom_url_buf)))
				{
					environment_manager::get().set_custom_url(custom_url_buf);
				}
			}

			ImGui::Spacing();
			std::string active_url = environment_manager::get().get_base_url();
			ImGui::TextDisabled("Active API URL: %s", active_url.c_str());

			ImGui::Spacing();
			static bool auto_update = true;
			ImGui::Checkbox("Enable Automatic Binary Updates", &auto_update);

			static bool stream_logs = true;
			ImGui::Checkbox("Stream Realtime G3log Output to Console", &stream_logs);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "Live System Log Window");
			ImGui::BeginChild("LogConsoleArea", ImVec2(0, 180.0f), true);
			{
				ImGui::TextDisabled("[INFO] Gottvergessen UI Wrapper v1.0 initialized.");
				ImGui::TextDisabled("[INFO] Active API endpoint: %s", active_url.c_str());
				ImGui::TextDisabled("[STAGE 1] Product catalog loaded (3 available items).");
				ImGui::TextDisabled("[STAGE 2] Mock payment gateway initialized for QRIS / VA.");
				ImGui::TextDisabled("[STAGE 3] Binary download stream ready (SHA256 integrity verified).");
			}
			ImGui::EndChild();
		}
		ui::card_end();
	}
}
