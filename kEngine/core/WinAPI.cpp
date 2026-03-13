#include "WinAPI.h"
#ifdef USE_IMGUI
#include "ImGuiManager.h"
#endif

WinAPI::~WinAPI() {
}

LRESULT CALLBACK WinAPI::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

#ifdef USE_IMGUI
	if (ImGuiManager::HandleMessage(hwnd, msg, wparam, lparam)) {
		return true;
	}
#endif

	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool WinAPI::Initialize(const std::string& kClientTitle, int kClientWidth, int kClientHeight, HINSTANCE hInstance) {
	className_ = ConvertString::SwitchStdStringWstring(kClientTitle);
	hInstance_ = hInstance;
	if (!RegisterWindowClass())										return false;
	if (!CreateMainWindow(kClientWidth, kClientHeight, hInstance))	return false;
	ShowWindow(hwnd_, SW_SHOW);
	return true;
}

void WinAPI::Finalize() const {
	//DestroyWindow(hwnd_);
	CoUninitialize();
}

bool WinAPI::ProcessMessage() {
	MSG msg{};

	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT) {
			return false;
		}
	}
	return true;
}

bool WinAPI::RegisterWindowClass() {
	///windowを登録する
	WNDCLASS wc{};
	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = className_.c_str();
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	return RegisterClass(&wc) != 0;
};

bool WinAPI::CreateMainWindow(int kClientWidth, int kClientHeight, HINSTANCE hInstance) {

	RECT wrc{ 0,0,kClientWidth ,kClientHeight };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, FALSE);

	hwnd_ = CreateWindow(
		className_.c_str(),
		className_.c_str(),
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left, // ウィンドウ横幅
		wrc.bottom - wrc.top, // ウィンドウ縦幅
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	return hwnd_ != nullptr;

}