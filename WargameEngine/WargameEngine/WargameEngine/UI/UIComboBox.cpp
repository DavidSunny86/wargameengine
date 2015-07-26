#include "UIComboBox.h"
#include "UIText.h"

CUIComboBox::CUIComboBox(int x, int y, int height, int width, IUIElement * parent, IRenderer & renderer) : CUIElement(x, y, height, width, parent, renderer),
m_selected(-1), m_expanded(false), m_pressed(false), m_scrollbar(m_theme, renderer)
{
}

void CUIComboBox::Draw() const
{
	if (!m_visible)
		return;
	m_renderer.PushMatrix();
	m_renderer.Translate(GetX(), GetY(), 0);
	int realHeight = GetHeight();
	if (m_expanded) realHeight += m_theme->combobox.elementSize * m_items.size();
	if (!m_cache)
	{
		m_cache = move(m_renderer.RenderToTexture([this]() {
			m_renderer.SetColor(m_theme->defaultColor[0], m_theme->defaultColor[1], m_theme->defaultColor[2]);
			m_renderer.RenderArrays(RenderMode::RECTANGLES,
			{ CVector2i{ 0, 0 }, { 0, GetHeight() }, { GetWidth(), GetHeight() }, { GetWidth(), 0 } }, {});

			m_renderer.SetColor(m_theme->textfieldColor[0], m_theme->textfieldColor[1], m_theme->textfieldColor[2]);
			int borderSize = m_theme->combobox.borderSize;
			m_renderer.RenderArrays(RenderMode::RECTANGLES, { CVector2i(borderSize, borderSize), {borderSize, GetHeight() - borderSize},
			{GetWidth() - borderSize, GetHeight() - borderSize}, {GetWidth() - borderSize, borderSize} }, {});

			m_renderer.SetColor(m_theme->text.color[0], m_theme->text.color[1], m_theme->text.color[2]);
			if (m_selected >= 0)	PrintText(m_theme->combobox.borderSize, m_theme->combobox.borderSize, GetWidth(), GetHeight(), m_items[m_selected], m_theme->combobox.text);

			m_renderer.SetColor(0.6f, 0.6f, 0.6f);
			m_renderer.SetTexture(m_theme->texture);
			float * texCoords = m_expanded ? m_theme->combobox.expandedTexCoord : m_theme->combobox.texCoord;
			int firstX = GetWidth() - static_cast<int>(GetHeight() * m_theme->combobox.buttonWidthCoeff);
			m_renderer.RenderArrays(RenderMode::RECTANGLES,
			{ CVector2i(firstX, 0), { firstX, GetHeight() }, {GetWidth(), GetHeight()}, {GetWidth(), 0} },
			{ CVector2f(texCoords), {texCoords[0], texCoords[3]}, {texCoords[2], texCoords[3]}, {texCoords[2], texCoords[1]} });
			m_renderer.SetTexture("");

			if (m_expanded)
			{
				m_renderer.SetColor(m_theme->textfieldColor[0], m_theme->textfieldColor[1], m_theme->textfieldColor[2]);
				int totalHeight = GetHeight() + m_theme->combobox.elementSize * m_items.size();
				m_renderer.RenderArrays(RenderMode::RECTANGLES, { CVector2i(0, GetHeight()), { 0, totalHeight }, {GetWidth(), totalHeight}, {GetWidth(), GetHeight()} }, {});

				m_renderer.SetColor(m_theme->text.color[0], m_theme->text.color[1], m_theme->text.color[2]);
				for (size_t i = m_scrollbar.GetPosition() / m_theme->combobox.elementSize; i < m_items.size(); ++i)
				{
					if (GetHeight() + m_theme->combobox.elementSize * static_cast<int>(i) - m_scrollbar.GetPosition() > m_windowHeight) break;
					PrintText(m_theme->combobox.borderSize, GetHeight() + m_theme->combobox.elementSize * i - m_scrollbar.GetPosition(), GetWidth(), m_theme->combobox.elementSize, m_items[i], m_theme->combobox.text);
				}

				m_renderer.PushMatrix();
				m_renderer.Translate(0, GetHeight(), 0);
				m_scrollbar.Draw();
				m_renderer.PopMatrix();
			}
		}, GetWidth(), realHeight));
	}

	m_cache->Bind();
	m_renderer.RenderArrays(RenderMode::TRIANGLE_STRIP,
	{ CVector2i(0, 0),{ GetWidth(), 0 },{ 0, realHeight },{ GetWidth(), realHeight } },
	{ CVector2f(0.0f, 0.0f),{ 1.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f, 1.0f } });
	m_renderer.SetTexture("");

	CUIElement::Draw();
	m_renderer.PopMatrix();
}

bool CUIComboBox::LeftMouseButtonDown(int x, int y)
{
	if (!m_visible) return false;
	if(CUIElement::LeftMouseButtonDown(x, y))
		return true;
	Invalidate();
	if(PointIsOnElement(x, y))
	{
		if (m_expanded)
		{
			if (m_scrollbar.LeftMouseButtonDown(x - GetX(), y - GetY() - GetHeight())) return true;
		}
		m_pressed = true;
		return true;

	}
	return false;
}

bool CUIComboBox::LeftMouseButtonUp(int x, int y)
{
	if(!m_visible) return false;
	Invalidate();
	if(CUIElement::LeftMouseButtonUp(x, y))
	{
		m_pressed = false;
		return true;
	}
	if (m_expanded && m_scrollbar.LeftMouseButtonUp(x - GetX(), y - GetY() - GetHeight())) return true;
	if(PointIsOnElement(x, y))
	{
		if(m_pressed)
		{
			if(m_expanded && PointIsOnElement(x, y))
			{
				int index = (y - GetHeight() - GetY() + m_scrollbar.GetPosition()) / m_theme->combobox.elementSize;
				if(index >= 0) m_selected = index;
				if(m_onChange) m_onChange();
			}
			m_expanded = !m_expanded;
		}
		SetFocus();
		m_pressed = false;
		return true;
	}
	else
	{
		m_expanded = false;
	}
	m_pressed = false;
	return false;
}

void CUIComboBox::AddItem(std::wstring const& str)
{
	m_items.push_back(str);
	if(m_selected == -1)
	{
		m_selected = 0;
	}
	m_scrollbar.Update(m_windowHeight - GetX() - GetHeight(), m_theme->combobox.elementSize * (m_items.size() + 1), GetWidth(), m_theme->combobox.elementSize);
	Invalidate();
}

std::wstring const CUIComboBox::GetText() const
{
	return m_items[m_selected];
}

void CUIComboBox::SetSelected(size_t index)
{
	m_selected = index;
	Invalidate();
}

bool CUIComboBox::PointIsOnElement(int x, int y) const
{
	int height = GetHeight();
	if(m_expanded)
	{
		height += m_theme->combobox.elementSize * m_items.size();
	}
	if(x > GetX() && x < GetX() + GetWidth() && y > GetY() && y < GetY() + height)
		return true;
	return false;
}

void CUIComboBox::DeleteItem(size_t index)
{
	m_items.erase(m_items.begin() + index);
	if(m_selected == index) m_selected--;
	if(m_selected == -1 && !m_items.empty()) m_selected = 0;
	m_scrollbar.Update(m_windowHeight - GetX() - GetHeight(), m_theme->combobox.elementSize * (m_items.size() + 1), GetWidth(), m_theme->combobox.elementSize);
	Invalidate();
}

void CUIComboBox::SetText(std::wstring const& text)
{
	Invalidate();
	for(size_t i = 0; i < m_items.size(); ++i)
	{
		if(m_items[i] == text)
		{
			m_selected = i;
			return;
		}
	}
}

void CUIComboBox::Resize(int windowHeight, int windowWidth) 
{
	CUIElement::Resize(windowHeight, windowWidth);
	m_scrollbar.Update(m_windowHeight - GetX() - GetHeight(), m_theme->combobox.elementSize * (m_items.size() + 1), GetWidth(), m_theme->combobox.elementSize);
	Invalidate();
}

int CUIComboBox::GetSelectedIndex() const
{ 
	return m_selected; 
}

size_t CUIComboBox::GetItemsCount() const
{ 
	return m_items.size(); 
}

std::wstring CUIComboBox::GetItem(size_t index) const
{ 
	return m_items[index]; 
}

void CUIComboBox::ClearItems()
{ 
	m_items.clear(); 
	m_selected = -1; 
	Invalidate();
}

void CUIComboBox::SetOnChangeCallback(std::function<void()> const& onChange)
{ 
	m_onChange = onChange; 
}

void CUIComboBox::SetTheme(std::shared_ptr<CUITheme> theme) 
{ 
	m_theme = theme; 
	m_scrollbar = CUIScrollBar(theme, m_renderer); 
	Invalidate();
}

void CUIComboBox::OnMouseMove(int x, int y)
{
	if (m_visible && m_focused) m_focused->OnMouseMove(x, y);
	if (m_scrollbar.OnMouseMove(x - GetX(), y - GetY() - GetHeight()))
	{
		Invalidate();
	}
}
