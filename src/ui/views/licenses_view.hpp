#pragma once

#include "ui/ui.hpp"

namespace gottvergessen
{
	class licenses_view
	{
	public:
		static licenses_view& get()
		{
			static licenses_view instance;
			return instance;
		}

		static void render(ui* ui_instance)
		{
			get().render_impl(ui_instance);
		}

	private:
		licenses_view() = default;
		~licenses_view() = default;

		licenses_view(const licenses_view&) = delete;
		licenses_view& operator=(const licenses_view&) = delete;

		void render_impl(ui* ui_instance);
	};
}
