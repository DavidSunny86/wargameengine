#include "UIList.h"
#include "UIText.h"
#include <GL/glut.h>
#include "../view/TextureManager.h"

void CUIList::Draw() const
{
	if(!m_visible)
		return;
	glPushMatrix();
	glTranslatef(GetX(), GetY(), 0.0f);
	glColor3f(m_theme.defaultColor[0], m_theme.defaultColor[1], m_theme.defaultColor[2]);
	glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(0, GetHeight());
		glVertex2i(GetWidth(), GetHeight());
		glVertex2i(GetWidth(), 0);
	glEnd();
	glColor3f(m_theme.textfieldColor[0], m_theme.textfieldColor[1], m_theme.textfieldColor[2]);
	glBegin(GL_QUADS);
		glVertex2i(m_theme.list.borderSize, m_theme.list.borderSize);
		glVertex2i(m_theme.list.borderSize, GetHeight() - m_theme.list.borderSize);
		glVertex2i(GetWidth() - m_theme.list.borderSize, GetHeight() - m_theme.list.borderSize);
		glVertex2i(GetWidth() - m_theme.list.borderSize, m_theme.list.borderSize);
	glEnd();
	if(m_selected > -1)
	{
		glColor3f(0.2f, 0.2f, 1.0f);
		glBegin(GL_QUADS);
			glVertex2i(m_theme.list.borderSize, m_theme.list.borderSize + m_theme.list.elementSize * m_selected);
			glVertex2i(m_theme.list.borderSize, 2 * m_theme.list.borderSize + m_theme.list.elementSize * (m_selected + 1) );
			glVertex2i(GetWidth() - m_theme.list.borderSize, 2 * m_theme.list.borderSize + m_theme.list.elementSize * (m_selected + 1));
			glVertex2i(GetWidth() - m_theme.list.borderSize, m_theme.list.borderSize + m_theme.list.elementSize * m_selected);
		glEnd();
	}
	glColor3f(m_theme.text.color[0], m_theme.text.color[1], m_theme.text.color[2]);
	for (size_t i = m_scrollbar.GetPosition() / m_theme.list.elementSize; i < m_items.size(); ++i)
	{
		if (m_theme.list.borderSize + m_theme.list.elementSize * (i - m_scrollbar.GetPosition() / m_theme.list.elementSize) > GetHeight()) break;
		PrintText(m_theme.list.borderSize, m_theme.list.borderSize + m_theme.list.elementSize * (i - m_scrollbar.GetPosition() / m_theme.list.elementSize), GetWidth(), m_theme.list.text.fontSize, m_items[i], m_theme.text);
	}
	m_scrollbar.Draw();
	CUIElement::Draw();
	glPopMatrix();
}

bool CUIList::LeftMouseButtonDown(int x, int y)
{
	if (!m_visible) return false;
	if (CUIElement::LeftMouseButtonDown(x, y))
	{
		return true;
	}
	return m_scrollbar.LeftMouseButtonDown(x - GetX(), y - GetY());
}

bool CUIList::LeftMouseButtonUp(int x, int y)
{
	if(!m_visible) return false;
	if(CUIElement::LeftMouseButtonUp(x, y))
	{
		return true;
	}
	if (m_scrollbar.LeftMouseButtonUp(x - GetX(), y - GetY())) return true;
	if(PointIsOnElement(x, y))
	{
		unsigned int index = (y - GetY()) / m_theme.list.elementSize;
		if(index >= 0 && index < m_items.size()) m_selected = index;
		if(m_onChange) m_onChange();
		SetFocus();
		return true;
	}
	return false;
}

void CUIList::AddItem(std::string const& str)
{
	m_items.push_back(str);
	if(m_selected == -1)
	{
		m_selected = 0;
	}
	m_scrollbar.Update(GetHeight(), m_theme.list.elementSize * m_items.size(), GetWidth(), m_theme.list.elementSize);
}

std::string const CUIList::GetText() const
{
	return m_items[m_selected];
}

void CUIList::SetSelected(size_t index)
{
	m_selected = index;
}

void CUIList::DeleteItem(size_t index)
{
	m_items.erase(m_items.begin() + index);
	if(m_selected == index) m_selected--;
	if(m_selected == -1 && !m_items.empty()) m_selected = 0;
	m_scrollbar.Update(GetHeight(), m_theme.list.elementSize * m_items.size(), GetWidth(), m_theme.list.elementSize);
}

void CUIList::SetText(std::string const& text)
{
	for(size_t i = 0; i < m_items.size(); ++i)
	{
		if(m_items[i] == text)
		{
			m_selected = i;
			return;
		}
	}
}

void CUIList::Resize(int windowHeight, int windowWidth)
{
	CUIElement::Resize(windowHeight, windowWidth);
	m_scrollbar.Update(GetHeight(), m_theme.list.elementSize * m_items.size(), GetWidth(), m_theme.list.elementSize);
}