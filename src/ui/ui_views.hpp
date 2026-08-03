#pragma once

#include "ui/ui.hpp"

namespace gottvergessen
{
	class ui_views
	{
	public:
		static void render_dashboard_overview(ui* ui_instance);
		static void render_products_catalog(ui* ui_instance);
		static void render_checkout_order(ui* ui_instance);
		static void render_transaction_payment(ui* ui_instance);
		static void render_my_licenses(ui* ui_instance);
		static void render_binary_download(ui* ui_instance, class renderer* renderer_ptr);
		static void render_settings(ui* ui_instance);
	};
}
