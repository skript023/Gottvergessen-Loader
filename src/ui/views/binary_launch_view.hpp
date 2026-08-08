#pragma once

#include "ui/ui.hpp"

namespace gottvergessen
{
	class binary_launch_view
	{
	public:
		static binary_launch_view& get()
		{
			static binary_launch_view instance;
			return instance;
		}

		static void render(ui* ui_instance, class renderer* renderer_ptr)
		{
			get().render_impl(ui_instance, renderer_ptr);
		}

	private:
		binary_launch_view() = default;
		~binary_launch_view() = default;

		binary_launch_view(const binary_launch_view&) = delete;
		binary_launch_view& operator=(const binary_launch_view&) = delete;

		void render_impl(ui* ui_instance, class renderer* renderer_ptr);
	};
}
