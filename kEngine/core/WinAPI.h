#pragma once
#include <windows.h>
#include <cstdint>
#include <iostream>
#include <locale>
#include <codecvt>

#include <vector>
#include <string>
#include "StringManage\ConvertString.h"
#include "Logger.h"

class WinAPI
{
public:
	~WinAPI();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	bool Initialize(const std::string& title, int width, int height, HINSTANCE hInstance = GetModuleHandle(nullptr));
	void Finalize() const;
	HWND GetHWND() const { return hwnd_; }
	HINSTANCE GetHINSTANCE() const { return hInstance_; }

	bool ProcessMessage();

private:
	HINSTANCE hInstance_ = {};
	HWND hwnd_ = {};
	std::wstring className_ = L"DefultWindowName_";

	bool RegisterWindowClass();
	bool CreateMainWindow(int kClientWidth, int kClientHeight, HINSTANCE hInstance);
};

