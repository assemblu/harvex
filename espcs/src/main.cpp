#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <cmath>
#include <numbers>
#include <thread>
#include <d3d11.h>
#include <tchar.h>
#include <dwmapi.h>

#include <math/math.h>
#include <mem/mem.h>
#include <offsets/offsets.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
	{
		return 0L;
	}
	
	if (uMsg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0L;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show)
{
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = instance;
	wc.lpszClassName = L"Assemblu Overlay Class";

	RegisterClass(&wc);

	const HWND window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED, wc.lpszClassName, L"Assemblu", WS_POPUP, 0, 0, 1920, 1080, nullptr, nullptr, wc.hInstance, nullptr);
	
	SetLayeredWindowAttributes(window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);
	{
		RECT client_area{};
		GetClientRect(window, &client_area);
		RECT window_area{};
		GetWindowRect(window, &window_area);
		POINT diff{};
		ClientToScreen(window, &diff);

		const MARGINS margins{
			window_area.left + (diff.x - window_area.left),
			window_area.top + (diff.y - window_area.top),
			client_area.right,
			client_area.bottom
		};

		DwmExtendFrameIntoClientArea(window, &margins);
	}

	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.RefreshRate.Numerator = 60U;
	sd.BufferDesc.RefreshRate.Denominator = 1U;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1U;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2U;
	sd.OutputWindow = window;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	constexpr D3D_FEATURE_LEVEL levels[2]
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0
	};

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	IDXGISwapChain* swap_chain = nullptr;
	ID3D11RenderTargetView* render_target = nullptr;
	D3D_FEATURE_LEVEL level{};

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		levels,
		2U,
		D3D11_SDK_VERSION,
		&sd,
		&swap_chain,
		&device,
		&level,
		&context
	);

	ID3D11Texture2D* back_buffer{ nullptr };
	swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

	if (back_buffer)
	{
		device->CreateRenderTargetView(back_buffer, nullptr, &render_target);
		back_buffer->Release();
	}
	else
	{
		return 1;
	}

	ShowWindow(window, cmd_show);
	UpdateWindow(window);

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, context);


	const DWORD pid = mem::GetProcId(L"cs2.exe");
	if (pid == 0)
	{
		MessageBox(NULL, L"Failed to get PID", L"...", MB_OK);
		return 1;
	}

	const std::uintptr_t client = mem::GetModuleBaseAddress(pid, L"client.dll");
	if (client == 0)
	{
		MessageBox(NULL, L"Failed to get client base", L"Assemblu", MB_OK);
		return 1;
	}
	MessageBox(NULL, L"Injected!", L"...", MB_OK);




	bool running = true;
	while (running)
	{
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				running = false;
			}

			ImGui_ImplWin32_WndProcHandler(window, msg.message, msg.wParam, msg.lParam);
		}

		if (!running)
		{
			break;
		}

		//const auto local_player_pawn = mem::Read<std::uintptr_t>(client + offsets::dwLocalPlyerPawn);
		//if (!local_player_pawn)
		//	MessageBox(NULL, L"local player not found", L"...", MB_OK);

		//const auto entity_list = mem::Read(<std::uintptr_t>(clinet + offsets::dwEntityList);
		//if (!entity_list)
		//	MessageBox(NULL, L"Entity list not found", L"...", MB_OK);

		//view_matrix_t view_matrix = mem::Read<view_matrix_t>(client + offsets::dwViewMatrix);
		//int local_team = mem::Read<view_matrix_t>(client + offsets::dwLocalTime);




		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Render();

		constexpr float color[4]{ 0.f, 0.f, 0.f, 0.f };
		context->OMSetRenderTargets(1U, &render_target, nullptr);
		context->ClearRenderTargetView(render_target, color);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		swap_chain->Present(1U, 0U);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	if (swap_chain)
	{
		swap_chain->Release();
	}
	
	if (context)
	{
		context->Release();
	}

	if (device)
	{
		device->Release();
	}

	if (render_target)
	{
		render_target->Release();
	}

	DestroyWindow(window);
	UnregisterClass(wc.lpszClassName, wc.hInstance);
	
	return 0;
}

