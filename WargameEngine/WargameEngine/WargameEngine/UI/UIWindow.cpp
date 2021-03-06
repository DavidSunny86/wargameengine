#include "UIWindow.h"
#include "../view/IRenderer.h"
#include "UIText.h"

namespace wargameEngine
{
namespace ui
{
CUIWindow::CUIWindow(int width, int height, std::wstring const& headerText, IUIElement* parent, view::ITextWriter& textWriter)
	: UIElement(parent->GetWidth() / 2, parent->GetHeight() / 2, height, width, parent, textWriter)
	, m_headerText(headerText)
	, m_dragging(false)
{
	m_x = (600 - m_width) / 2;
	m_y = (600 - m_height) / 2;
}

void CUIWindow::Draw(view::IRenderer& renderer) const
{
	if (!m_visible)
		return;
	renderer.PushMatrix();
	renderer.Translate(GetX(), GetY(), 0);
	auto& theme = m_theme->window;
	if (!m_cache)
	{
		m_cache = renderer.CreateTexture(nullptr, GetWidth(), GetHeight(), CachedTextureType::RenderTarget);
	}
	if (m_invalidated)
	{
		renderer.RenderToTexture([this, &renderer]() {
			renderer.UnbindTexture();
			auto& theme = m_theme->window;
			renderer.SetColor(theme.headerColor);
			int headerHeight = static_cast<int>(theme.headerHeight * m_scale);
			renderer.RenderArrays(RenderMode::TriangleStrip,
				{ CVector2i(0, 0), { 0, headerHeight }, { GetWidth(), 0 }, { GetWidth(), headerHeight } }, {});
			renderer.SetTexture(m_theme->texture);
			int buttonSize = static_cast<int>(theme.buttonSize * m_scale);
			int right = GetWidth() - buttonSize;
			float* texCoord = theme.closeButtonTexCoord;
			renderer.RenderArrays(RenderMode::TriangleStrip,
				{ CVector2i(right, 0), { right, buttonSize }, { GetWidth(), 0 }, { GetWidth(), buttonSize } },
				{ CVector2f(texCoord[0], texCoord[1]), { texCoord[0], texCoord[3] }, { texCoord[2], texCoord[1] }, { texCoord[2], texCoord[3] } });
			renderer.UnbindTexture();
			renderer.SetColor(m_theme->defaultColor);
			renderer.RenderArrays(RenderMode::TriangleStrip,
				{ CVector2i(0, headerHeight), { GetWidth(), headerHeight }, { 0, GetHeight() }, { GetWidth(), GetHeight() }, }, {});
			PrintText(renderer, m_textWriter, 0, 0, GetWidth() - buttonSize, headerHeight, m_headerText, theme.headerText, m_scale);

		}, *m_cache, GetWidth(), GetHeight());
	}
	renderer.SetTexture(*m_cache);
	renderer.RenderArrays(RenderMode::TriangleStrip,
		{ CVector2i(0, 0), { GetWidth(), 0 }, { 0, GetHeight() }, { GetWidth(), GetHeight() } },
		{ CVector2f(0.0f, 0.0f), { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } });
	renderer.Translate(0, theme.headerHeight, 0);
	UIElement::Draw(renderer);
	renderer.PopMatrix();
}

bool CUIWindow::LeftMouseButtonDown(int x, int y)
{
	if (!m_visible || !PointIsOnElement(x, y))
		return UIElement::LeftMouseButtonDown(x, y - m_theme->window.headerHeight);
	int relativeY = y - m_theme->window.headerHeight;
	if (relativeY - GetY() < 0)
	{
		m_dragging = true;
		m_prevX = x;
		m_prevY = y;
		m_parent->SetFocus(this);
	}
	else
	{
		UIElement::LeftMouseButtonDown(x, relativeY);
	}
	return true;
}

bool CUIWindow::LeftMouseButtonUp(int x, int y)
{
	auto& theme = m_theme->window;
	if (!m_visible || !PointIsOnElement(x, y))
		return UIElement::LeftMouseButtonUp(x, y - theme.headerHeight);
	int relativeY = y - theme.headerHeight;
	if (relativeY - GetY() < 0)
	{
		if (GetWidth() - (x - GetX()) < theme.buttonSize)
		{
			m_parent->SetFocus();
			m_parent->DeleteChild(this); //may cause problems
			return true;
		}
	}
	else
	{
		UIElement::LeftMouseButtonUp(x, relativeY);
	}
	m_dragging = false;
	return true;
}

void CUIWindow::OnMouseMove(int x, int y)
{
	if (m_dragging)
	{
		m_x += x - m_prevX;
		m_y += y - m_prevY;
		m_prevX = x;
		m_prevY = y;
		Invalidate();
	}
	UIElement::OnMouseMove(x, y);
}

int CUIWindow::GetHeight() const
{
	return UIElement::GetWidth() + m_theme->window.headerHeight * m_windowWidth / 600;
}

int CUIWindow::GetX() const
{
	return m_x;
}

int CUIWindow::GetY() const
{
	return m_y;
}
}
}