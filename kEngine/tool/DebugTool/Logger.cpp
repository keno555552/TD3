#include "Logger.h"
#include <Windows.h>

void Logger::Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}

void Logger::Log(const std::wstring& message) {
	OutputDebugStringW(message.c_str());
}

void Logger::LogUtf8(const std::string& message){
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
    if (wideSize <= 0) return;

    std::vector<wchar_t> wideBuf(wideSize);
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, wideBuf.data(), wideSize);

    OutputDebugStringW(wideBuf.data());
    OutputDebugStringW(L"\n");
}
