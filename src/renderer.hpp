#pragma once
#include "common.hpp"

namespace gottvergessen
{
	class renderer
	{
	public:
        explicit renderer();
        ~renderer();

		void on_present();
		bool create_d3d_device(HWND hwnd);
		void clear_d3d();
		void clear_all();
		void reset_device();
		bool init();
		static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	public:
		WNDCLASSEX m_window_class;
		HWND m_hwnd;
		LPCSTR m_name;
	public:
		LPDIRECT3D9 m_d3d9 = nullptr;
		LPDIRECT3DDEVICE9 m_device = nullptr;
		D3DPRESENT_PARAMETERS m_parameters = { NULL };
		MSG m_message = { nullptr };
	};

	inline renderer* g_renderer{};
}