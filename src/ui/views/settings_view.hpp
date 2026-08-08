#pragma once

#include "ui/ui.hpp"

namespace gottvergessen
{
	class settings_view
	{
	public:
		static settings_view& get()
		{
			static settings_view instance;
			return instance;
		}

		static void render(ui* ui_instance)
		{
			get().render_impl(ui_instance);
		}

	private:
		settings_view() = default;
		~settings_view() = default;

		settings_view(const settings_view&) = delete;
		settings_view& operator=(const settings_view&) = delete;

		void render_impl(ui* ui_instance);
	};
}
