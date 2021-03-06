#include "Utils.h"
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>

namespace wargameEngine
{
std::wstring Utf8ToWstring(std::string const& str)
{
	std::wstring result;
	result.resize(str.size());
	mbstowcs(&result[0], str.c_str(), str.size());
	return result;
}

std::string WStringToUtf8(std::wstring const& str)
{
	std::string result;
	result.resize(str.size());
	wcstombs(&result[0], str.c_str(), str.size());
	return result;
}

std::wstring ReplaceAll(std::wstring const& text, std::unordered_map<std::wstring, std::wstring> const& replaceMap)
{
	std::wstring result = text;
	for (auto& p : replaceMap)
	{
		size_t pos = result.find(p.first);
		while (pos != result.npos)
		{
			result.replace(pos, p.first.size(), p.second);
			pos = result.find(p.first, pos + p.second.size());
		}
	}
	return result;
}

std::string to_string(const Path& path)
{
#ifdef _WINDOWS
	return WStringToUtf8(path);
#else
	return path;
#endif
}

std::wstring to_wstring(const Path& path)
{
#ifdef _WINDOWS
	return path;
#else
	return Utf8ToWstring(path);
#endif
}

std::vector<char> ReadFile(const Path& path)
{
	std::vector<char> result;
	std::ifstream file(path, std::ios::binary | std::ios::in);
	if (!file)
		return result;
	file.seekg(0, std::ios::end);
	std::streamsize size = file.tellg();
	result.resize(static_cast<size_t>(size));
	file.seekg(0, std::ios::beg);
	file.read(result.data(), size);
	return result;
}

void WriteFile(const Path& path, const char* data, size_t size)
{
	std::ofstream file(path, std::ios::binary | std::ios::out);
	file.write(data, size);
}

void WriteFile(const Path& path, const std::vector<char>& data)
{
	WriteFile(path, data.data(), data.size());
}

std::wstring ToWstring(double value, size_t precision /*= 0*/)
{
	std::wostringstream out;
	out << std::setprecision(precision) << std::fixed << value;
	return out.str();
}
}