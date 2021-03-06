#pragma once
#include <string>

namespace wargameEngine
{
namespace view
{
class IRenderer;

class ITextWriter
{
public:
	virtual ~ITextWriter() {}

	virtual void Reset() = 0;
	virtual void PrintText(IRenderer& renderer, int x, int y, const std::string& font, unsigned int size, const std::string& text, int width = 0, int height = 0) = 0;
	virtual void PrintText(IRenderer& renderer, int x, int y, const std::string& font, unsigned int size, const std::wstring& text, int width = 0, int height = 0) = 0;
	virtual int GetStringHeight(const std::string& font, unsigned int size, const std::string& text) = 0;
	virtual int GetStringWidth(const std::string& font, unsigned int size, const std::string& text) = 0;
	virtual int GetStringHeight(const std::string& font, unsigned int size, const std::wstring& text) = 0;
	virtual int GetStringWidth(const std::string& font, unsigned int size, const std::wstring& text) = 0;
};
}
}