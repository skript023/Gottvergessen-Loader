#include "renderer.hpp"
#include "gui.hpp"

#include <backends/imgui_impl_dx9.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>
#include <imgui_internal.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace gottvergessen
{
	renderer::renderer()
	{
        g_renderer = this;
        if (this->init())
        {
            g_gui.dx_init();
            LOG(HACKER) << "Renderer initialized.";
        }
	}

    renderer::~renderer()
    {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        DestroyWindow(m_hwnd);
        clear_all();

        g_renderer = nullptr;
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

            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            {
                g_gui.dx_on_tick();
            }
            ImGui::EndFrame();

            m_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
            if (m_device->BeginScene() >= 0)
            {
                ImGui::Render();
                ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
                m_device->EndScene();
            }

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            HRESULT Result = m_device->Present(NULL, NULL, NULL, NULL);
            if (Result == D3DERR_DEVICELOST && m_device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            {
                reset_device();
            }
            if (!g_gui.m_opened)
            {
                m_message.message = WM_QUIT;
            }

        }
    }

    bool renderer::create_d3d_device(HWND hwnd)
    {
        if ((m_d3d9 = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        {
            return false;
        }
        ZeroMemory(&m_parameters, sizeof(m_parameters));
        m_parameters.Windowed = TRUE;
        m_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        m_parameters.BackBufferFormat = D3DFMT_UNKNOWN;
        m_parameters.EnableAutoDepthStencil = TRUE;
        m_parameters.AutoDepthStencilFormat = D3DFMT_D16;
        m_parameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        if (m_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_parameters, &m_device) < 0) 
        {
            return false;
        }
        return true;
    }

    void renderer::clear_d3d()
    {
        if (m_device) 
        {
            m_device->Release();
            m_device = NULL;
        }

        if (m_d3d9) 
        {
            m_d3d9->Release();
            m_d3d9 = NULL;
        }
    }

    void renderer::clear_all()
    {
        clear_d3d();
        UnregisterClass(m_window_class.lpszClassName, m_window_class.hInstance);
    }

    void renderer::reset_device()
    {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        HRESULT hr = m_device->Reset(&m_parameters);
        if (hr == D3DERR_INVALIDCALL)
        {
            IM_ASSERT(0);
        }
        ImGui_ImplDX9_CreateDeviceObjects();
    }

    bool renderer::init()
    {
        m_name = "Gottvergessen";
        m_window_class =
        {
            sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, m_name, NULL
        };

        RegisterClassEx(&m_window_class);
        m_hwnd = CreateWindow(m_name, m_name, WS_POPUP, 0, 0, 5, 5, NULL, NULL, m_window_class.hInstance, NULL);
        if (!create_d3d_device(m_hwnd))
        {
            clear_all();
            return false;
        }

        ShowWindow(m_hwnd, SW_HIDE);
        UpdateWindow(m_hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplWin32_Init(m_hwnd);
        ImGui_ImplDX9_Init(m_device);
        ZeroMemory(&m_message, sizeof(m_message));

        return true;
    }

    LRESULT renderer::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg)
        {
        case WM_SIZE:
            if (g_renderer->m_device != NULL && wParam != SIZE_MINIMIZED)
            {
                g_renderer->m_parameters.BackBufferWidth = LOWORD(lParam);
                g_renderer->m_parameters.BackBufferHeight = HIWORD(lParam);
                g_renderer->reset_device();
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