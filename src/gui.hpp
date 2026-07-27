#pragma once
#include "common.hpp"

namespace gottvergessen
{
	class gui
	{
	public:
		void dx_init();
		void dx_on_tick(class renderer* renderer);
	public:
		bool m_opened = true;
	private:
		bool m_init_pos{ false };
	};

	inline gui g_gui{};
}