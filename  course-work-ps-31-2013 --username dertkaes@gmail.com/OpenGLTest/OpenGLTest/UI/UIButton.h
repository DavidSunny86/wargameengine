#include "UIElement.h"

class CUIButton : public CUIElement
{
public:
	CUIButton(int x, int y, int height, int width, char* text, callback(onClick), IUIElement * parent): 
		CUIElement(x, y, height, width, parent), m_text(text), m_onClick(onClick), m_isPressed(false) {}
	void Draw() const;
	bool LeftMouseButtonUp(int x, int y);
	bool LeftMouseButtonDown(int x, int y);
	std::string const GetText() const { return m_text; }
	void SetText(std::string const& text) { m_text = text; }
private:
	std::string m_text;
	callback(m_onClick);
	bool m_isPressed;
};