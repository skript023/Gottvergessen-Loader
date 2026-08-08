#pragma once

#include "ui/ui.hpp"
#include "ui/views/binary_launch_view.hpp"
#include "ui/views/licenses_view.hpp"
#include "ui/views/settings_view.hpp"

namespace gottvergessen
{
	class ui_views
	{
	public:
		static void render_binary_download(ui* ui_instance, class renderer* renderer_ptr)
		{
			binary_launch_view::render(ui_instance, renderer_ptr);
		}

		static void render_my_licenses(ui* ui_instance)
		{
			licenses_view::render(ui_instance);
		}

		static void render_settings(ui* ui_instance)
		{
			settings_view::render(ui_instance);
		}
	};
}
