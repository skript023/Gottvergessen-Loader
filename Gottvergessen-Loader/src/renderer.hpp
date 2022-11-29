#pragma once
#include "common.hpp"

namespace gottvergessen
{
	class renderer
	{
		struct OverlayWindow
		{
			WNDCLASSEX m_window_class;
			HWND m_hwnd;
			LPCSTR m_name;
		};

		struct DirectX9Interface
		{
			LPDIRECT3D9 IDirect3D9 = NULL;
			LPDIRECT3DDEVICE9 pDevice = NULL;
			D3DPRESENT_PARAMETERS pParameters = { NULL };
			MSG Message = { NULL };
		};
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
		static void script_exception_handler(PEXCEPTION_POINTERS exp);
	public:
		OverlayWindow m_overlay;
		DirectX9Interface m_directx9;
	};

	inline renderer* g_renderer{};

#define TRY_CLAUSE  __try
#define EXCEPT_CLAUSE  __except (renderer::script_exception_handler(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER) { }
}