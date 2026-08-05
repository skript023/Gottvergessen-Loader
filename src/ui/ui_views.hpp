#pragma once

#include "ui/ui.hpp"

namespace gottvergessen
{
	class ui_views
	{
	public:
		static void render_binary_download(ui* ui_instance, class renderer* renderer_ptr);
		static void render_my_licenses(ui* ui_instance);
		static void render_settings(ui* ui_instance);
	};
}
