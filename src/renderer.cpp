#include "renderer.hpp"

#include "gui.hpp"
#include "fonts/font_list.hpp"
#include "fonts/icon_list.hpp"
#include "images/images.hpp"
#include "textures/textures.hpp"
#include "classes/resolution.hpp"

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>
#include <imgui_internal.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace gottvergessen
{
	renderer::renderer()
    {
        if (this->init())
        {
            g_gui.dx_init();
        }

        g_renderer = this;
    }

    renderer::~renderer()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        
        if (m_hwnd)
            DestroyWindow(m_hwnd);

        clear_all();
        g_renderer = nullptr;
    }

    bool renderer::create_d3d_device(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;
        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0,
        };

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            createDeviceFlags,
            featureLevelArray,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &m_swap_chain,
            &m_device,
            &featureLevel,
            &m_device_context
        );

        if (FAILED(hr))
            return false;

        create_render_target();
        return true;
    }

    void renderer::create_render_target()
    {
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(m_swap_chain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))))
        {
            m_device->CreateRenderTargetView(pBackBuffer, NULL, &m_render_target_view);
            pBackBuffer->Release();
        }
    }

    void renderer::cleanup_render_target()
    {
        if (m_render_target_view)
        {
            m_render_target_view->Release();
            m_render_target_view = nullptr;
        }
    }

    void renderer::clear_d3d()
    {
        cleanup_render_target();

        if (m_swap_chain) { m_swap_chain->Release(); m_swap_chain = nullptr; }
        if (m_device_context) { m_device_context->Release(); m_device_context = nullptr; }
        if (m_device) { m_device->Release(); m_device = nullptr; }
    }

    void renderer::clear_all()
    {
        clear_d3d();
        UnregisterClass(m_window_class.lpszClassName, m_window_class.hInstance);
    }

    bool renderer::init()
    {
        m_name = "Gottvergessen";
        m_window_class = {
            sizeof(WNDCLASSEX), CS_CLASSDC | CS_HREDRAW | CS_VREDRAW, wndproc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, m_name, NULL
        };

        auto screen_res = ScreenResolution(::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));

        RegisterClassEx(&m_window_class);

        // Ukuran fisik awal HWND (600x400) dan diposisikan tepat di tengah layar
        int win_w = 600;
        int win_h = 400;
        int win_x = (screen_res.x - win_w) / 2;
        int win_y = (screen_res.y - win_h) / 2;

        // Menggunakan WS_POPUP | WS_THICKFRAME tanpa WS_EX_LAYERED agar DirectX 11 render solid & tidak hilang
        m_hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED, m_window_class.lpszClassName, m_name, WS_POPUP,
            win_x, win_y, win_w, win_h, NULL, NULL, m_window_class.hInstance, NULL);

        SetWindowLong(m_hwnd, GWL_EXSTYLE, GetWindowLong(m_hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TOPMOST);
        SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

        {
            RECT client_area{};
            GetClientRect(m_hwnd, &client_area);

            RECT window_area{};
            GetWindowRect(m_hwnd, &window_area);

            POINT diff{};
            ClientToScreen(m_hwnd, &diff);

            // Menggunakan margin 1px agar Windows 11 memberikan efek rounded corner & shadow tanpa latar putih
            const MARGINS margins = { -1, -1, -1, -1 };
            DwmExtendFrameIntoClientArea(m_hwnd, &margins);
        }
        
        if (!create_d3d_device(m_hwnd))
        {
            clear_all();
            return false;
        }

        ShowWindow(m_hwnd, SW_SHOWDEFAULT);
        UpdateWindow(m_hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        
        // Nonaktifkan ViewportsEnable agar ImGui tidak membuat jendela OS sekunder yang berwarna putih
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = NULL;

        ImGui_ImplWin32_Init(m_hwnd);
        ImGui_ImplDX11_Init(m_device, m_device_context);
        ZeroMemory(&m_message, sizeof(m_message));

        ImFontConfig font_cfg{};
        font_cfg.FontDataOwnedByAtlas = false;
        std::strcpy(font_cfg.Name, "Rubik");

        m_font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(font_rubik), sizeof(font_rubik), 17.f, &font_cfg);

        ImFontConfig chinese_cfg{};
        chinese_cfg.MergeMode = true;
        chinese_cfg.PixelSnapH = true;

        ImGui::GetIO().Fonts->AddFontFromFileTTF(
            "C:\\Windows\\Fonts\\msyh.ttc",
            17.f,
            &chinese_cfg,
            ImGui::GetIO().Fonts->GetGlyphRangesChineseFull()
        );

        ImFontConfig russian_cfg{};
        russian_cfg.MergeMode = true;      // Merge dengan font sebelumnya
        russian_cfg.PixelSnapH = true;

        // Menambahkan font Cyrillic
        ImGui::GetIO().Fonts->AddFontFromFileTTF(
            "C:\\Windows\\Fonts\\segoeui.ttf",   // Bisa pakai Arial, Segoe UI, Roboto, dll
            17.0f,                             // Ukuran font
            &russian_cfg,
            ImGui::GetIO().Fonts->GetGlyphRangesCyrillic()  // Range karakter Rusia
        );

        merge_icon_with_latest_font(14.f, false);

        m_monospace_font = ImGui::GetIO().Fonts->AddFontDefault();

        ImGui::GetIO().Fonts->Build();

        if (!textures::load_from_memory(quantum_icons, _ARRAYSIZE(quantum_icons), m_device, &m_icons, &m_icons_size.x, &m_icons_size.y))
        {
            LOG(WARNING) << "Failed load textures";
        }

        return true;
    }

    void renderer::on_present()
    {
        while (m_message.message != WM_QUIT)
        {
            if (PeekMessage(&m_message, NULL, 0U, 0U, PM_REMOVE))
            {
                TranslateMessage(&m_message);
                DispatchMessage(&m_message);
                continue;
            }

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            {
                g_gui.dx_on_tick(this);
            }
            ImGui::EndFrame();

            // [PENTING] Clear Render Target dengan Warna Hitam 100% Transparan (Alpha = 0.0f)
            const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            m_device_context->OMSetRenderTargets(1, &m_render_target_view, NULL);
            m_device_context->ClearRenderTargetView(m_render_target_view, clear_color);

            ImGui::Render();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            m_swap_chain->Present(1, 0);

            if (!g_gui.m_opened)
            {
                m_message.message = WM_QUIT;
            }
        }
    }

    void renderer::merge_icon_with_latest_font(float font_size, bool FontDataOwnedByAtlas)
    {
        static const ImWchar icons_ranges[3] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.FontDataOwnedByAtlas = FontDataOwnedByAtlas;

        m_font_icon = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)font_icons, sizeof(font_icons), font_size, &icons_config, icons_ranges);
    }

    LRESULT renderer::wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg)
        {
        case WM_SIZE:
            if (g_renderer && g_renderer->m_device != NULL && wParam != SIZE_MINIMIZED)
            {
                g_renderer->cleanup_render_target();
                g_renderer->m_swap_chain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                g_renderer->create_render_target();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)
                return 0;
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}