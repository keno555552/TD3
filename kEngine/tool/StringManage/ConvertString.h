#pragma once
#include <windows.h>
#include <string>

class ConvertString {
public:

	/// <summary>
	/// std::string <-> std::wstring
	/// </summary>
	/// <param name="str">string</param>
	/// <returns>wstring</returns>
	static std::wstring SwitchStdStringWstring(const std::string& str);
	/// <summary>
	/// std::string <-> std::wstring
	/// </summary>
	/// <param name="str">wstring</param>
	/// <returns>string</returns>
	static std::string SwitchStdStringWstring(const std::wstring& str);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="utf8"></param>
	/// <returns></returns>
	static std::wstring SwitchStdUtf8Utf16(const std::string& utf8);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="utf8"></param>
	/// <returns></returns>
	static std::string SwitchStdUtf8Utf16(const std::wstring& wide);

};