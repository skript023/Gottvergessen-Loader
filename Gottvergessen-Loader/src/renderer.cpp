#include "renderer.hpp"
#include "gui.hpp"

#include "docking_imgui/imgui.h"
#include "docking_imgui/imgui_internal.h"
#include "docking_imgui/imgui_impl_dx9.h"
#include "docking_imgui/imgui_impl_win32.h"

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
        DestroyWindow(m_overlay.m_hwnd);
        clear_all();

        g_renderer = nullptr;
    }

    void renderer::on_present()
    {
        while (m_directx9.Message.message != WM_QUIT)
        {
            if (PeekMessage(&m_directx9.Message, NULL, 0U, 0U, PM_REMOVE))
            {
                TranslateMessage(&m_directx9.Message);
                DispatchMessage(&m_directx9.Message);
                continue;
            }

            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            {
                TRY_CLAUSE
                {
                    g_gui.dx_on_tick();
                } EXCEPT_CLAUSE
            }
            ImGui::EndFrame();

            m_directx9.pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
            if (m_directx9.pDevice->BeginScene() >= 0)
            {
                ImGui::Render();
                ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
                m_directx9.pDevice->EndScene();
            }

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            HRESULT Result = m_directx9.pDevice->Present(NULL, NULL, NULL, NULL);
            if (Result == D3DERR_DEVICELOST && m_directx9.pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            {
                reset_device();
            }
            if (!g_gui.m_opened)
            {
                m_directx9.Message.message = WM_QUIT;
            }

        }
    }

    bool renderer::create_d3d_device(HWND hwnd)
    {
        if ((m_directx9.IDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        {
            return false;
        }
        ZeroMemory(&m_directx9.pParameters, sizeof(m_directx9.pParameters));
        m_directx9.pParameters.Windowed = TRUE;
        m_directx9.pParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        m_directx9.pParameters.BackBufferFormat = D3DFMT_UNKNOWN;
        m_directx9.pParameters.EnableAutoDepthStencil = TRUE;
        m_directx9.pParameters.AutoDepthStencilFormat = D3DFMT_D16;
        m_directx9.pParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        if (m_directx9.IDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_directx9.pParameters, &m_directx9.pDevice) < 0) 
        {
            return false;
        }
        return true;
    }

    void renderer::clear_d3d()
    {
        if (m_directx9.pDevice) 
        {
            m_directx9.pDevice->Release();
            m_directx9.pDevice = NULL;
        }

        if (m_directx9.IDirect3D9) 
        {
            m_directx9.IDirect3D9->Release();
            m_directx9.IDirect3D9 = NULL;
        }
    }

    void renderer::clear_all()
    {
        clear_d3d();
        UnregisterClass(m_overlay.m_window_class.lpszClassName, m_overlay.m_window_class.hInstance);
    }

    void renderer::reset_device()
    {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        HRESULT hr = m_directx9.pDevice->Reset(&m_directx9.pParameters);
        if (hr == D3DERR_INVALIDCALL)
        {
            IM_ASSERT(0);
        }
        ImGui_ImplDX9_CreateDeviceObjects();
    }
    bool renderer::init()
    {
        m_overlay.m_name = "Gottvergessen";
        m_overlay.m_window_class =
        {
            sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, m_overlay.m_name, NULL
        };

        RegisterClassEx(&m_overlay.m_window_class);
        m_overlay.m_hwnd = CreateWindow(m_overlay.m_name, m_overlay.m_name, WS_POPUP, 0, 0, 5, 5, NULL, NULL, m_overlay.m_window_class.hInstance, NULL);
        if (!create_d3d_device(m_overlay.m_hwnd))
        {
            clear_all();
            return false;
        }

        ShowWindow(m_overlay.m_hwnd, SW_HIDE);
        UpdateWindow(m_overlay.m_hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplWin32_Init(m_overlay.m_hwnd);
        ImGui_ImplDX9_Init(m_directx9.pDevice);
        ZeroMemory(&m_directx9.Message, sizeof(m_directx9.Message));

        return true;
    }

    LRESULT renderer::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg)
        {
        case WM_SIZE:
            if (g_renderer->m_directx9.pDevice != NULL && wParam != SIZE_MINIMIZED)
            {
                g_renderer->m_directx9.pParameters.BackBufferWidth = LOWORD(lParam);
                g_renderer->m_directx9.pParameters.BackBufferHeight = HIWORD(lParam);
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
    void renderer::script_exception_handler(PEXCEPTION_POINTERS exp)
    {
        HMODULE mod{};
        DWORD64 offset{};
        char buffer[MAX_PATH]{};
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)exp->ExceptionRecord->ExceptionAddress, &mod) == TRUE && mod != nullptr)
        {
            offset = ((DWORD64)exp->ExceptionRecord->ExceptionAddress - (DWORD64)mod);
            GetModuleFileNameA(mod, buffer, MAX_PATH - 1);
        }
        LOG(FATAL) << "Exception Code: " << HEX_TO_UPPER(exp->ExceptionRecord->ExceptionCode) << " Exception Offset: " << HEX_TO_UPPER(offset) << " Fault Module Name: " << buffer;
    }
}