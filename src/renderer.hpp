#pragma once
#include "common.hpp"
#include "imgui.h"

namespace gottvergessen
{
	class renderer
	{
		ImFont* m_font{};
		ImFont* m_monospace_font{};
		ImFont* m_font_icon{};
	public:
        explicit renderer();
        ~renderer();

		void on_present();
        void merge_icon_with_latest_font(float font_size, bool FontDataOwnedByAtlas);
        bool create_d3d_device(HWND hwnd);
        void clear_d3d();
		void clear_all();
		void reset_device();
		bool init();

		void create_render_target();
     	void cleanup_render_target();
		static LRESULT wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	public:
		WNDCLASSEX m_window_class;
		HWND m_hwnd;
		LPCSTR m_name;
	public:
		ID3D11Device* m_device = nullptr;
		ID3D11DeviceContext* m_device_context = nullptr;
		IDXGISwapChain* m_swap_chain = nullptr;
		ID3D11RenderTargetView* m_render_target_view = nullptr;
		MSG m_message = { nullptr };
	};

	inline renderer* g_renderer{};
}