#pragma once
#include <cstdint>
#include <iostream>
#include <vector>

#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")

#include <dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")

class Logger
{
public:
	/// <summary>
	/// 出力ウィンドウに文字出力
	/// </summary>
	/// <param name="message"></param>
	static void Log(const std::string& message);

	/// <summary>
	/// 出力ウィンドウに文字出力
	/// </summary>
	/// <param name="message"></param>
	static void Log(const std::wstring& message);

	/// <summary>
	/// 出力ウィンドウに文字出力
	/// </summary>
	/// <typeparam name="...Args"></typeparam>
	/// <param name="fmt"></param>
	/// <param name="...args"></param>
	template<typename... Args>
	static void Log(const char* fmt, Args... args) {
		int size = std::snprintf(nullptr, 0, fmt, args...) + 1;
		if (size <= 0) return;

		std::vector<char> buf(size);
		std::snprintf(buf.data(), size, fmt, args...);

		OutputDebugStringA(buf.data());
		OutputDebugStringA("\n");
	}

	/// <summary>
    /// UTF-8 の std::string をワイド文字列に変換して出力
    /// </summary>
	static void LogUtf8(const std::string& message);
};
