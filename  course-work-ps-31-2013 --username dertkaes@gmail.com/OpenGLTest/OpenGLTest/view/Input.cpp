#include "Input.h"
#include <GL\glut.h>
#include "GameView.h"
#include "..\SelectionTools.h"
#include "..\controller\CommandHandler.h"

bool CInput::m_isLMBDown = false;
bool CInput::m_ruler = false;
double CInput::startX = 0;
double CInput::startY = 0;

void CInput::OnMouse(int button, int state, int x, int y)
{
	switch(button)
	{
	case GLUT_LEFT_BUTTON: 
		if (state == GLUT_DOWN)
		{
			m_isLMBDown = true;
			if(CGameView::GetIntanse().lock()->UILeftMouseButtonDown(x, y)) return;
			if(m_ruler)
			{
				CGameView::GetIntanse().lock()->RulerBegin(x, y);
			}
			else
			{
				if(!CGameModel::IsGroup(CGameModel::GetIntanse().lock()->GetSelectedObject()))
					CGameView::GetIntanse().lock()->SelectObject(x, y);
				if(CGameModel::GetIntanse().lock()->GetSelectedObject().get())//drag object
				{
					startX = CGameModel::GetIntanse().lock()->GetSelectedObject()->GetX();
					startY = CGameModel::GetIntanse().lock()->GetSelectedObject()->GetY();
				}
				else//selection rectangle
				{
					startX = x;
					startY = y;
				}
				CGameView::GetIntanse().lock()->RulerHide();
			}
		}
		else
		{
			m_isLMBDown = false;
			if(CGameView::GetIntanse().lock()->UILeftMouseButtonUp(x, y)) return;
			if(!m_ruler && CGameModel::GetIntanse().lock()->GetSelectedObject())
			{
				double worldX, worldY;
				WindowCoordsToWorldCoords(x, y, worldX, worldY);
				CCommandHandler::GetInstance().lock()->AddNewMoveObject(worldX - startX, worldY - startY);
				startX = -1;
				startY = -1;
			}
			if(!CGameModel::GetIntanse().lock()->GetSelectedObject())
			{
				CGameView::GetIntanse().lock()->SelectObjectGroup(startX, startY, x, y);
			}
			m_ruler = false;
		}break;
	case SCROLL_UP:
		if (state == GLUT_UP)
		{
			CGameView::GetIntanse().lock()->CameraZoomIn();
		}break;
	case SCROLL_DOWN:
		if (state == GLUT_UP)
		{
			CGameView::GetIntanse().lock()->CameraZoomOut();
		}break;
	}
}

void CInput::OnKeyboard(unsigned char key, int x, int y)
{
	if(CGameView::GetIntanse().lock()->UIKeyPress(key))
		return;
	switch(key)
	{
		case BACKSPACE_BUTTON_ID:
		{
			CGameView::GetIntanse().lock()->CameraReset();
		}
		break;
	}
}

void CInput::OnSpecialKeyPress(int key, int x, int y)
{
    if(CGameView::GetIntanse().lock()->UISpecialKeyPress(key))
		return;
	switch (key) 
	{
		case GLUT_KEY_LEFT:
		{
			CGameView::GetIntanse().lock()->CameraTranslateLeft();
		}
		break;
		case GLUT_KEY_RIGHT:
		{
			CGameView::GetIntanse().lock()->CameraTranslateRight();
		}
		break;
		case GLUT_KEY_DOWN:
		{
			CGameView::GetIntanse().lock()->CameraTranslateDown();
		}
		break;
		case GLUT_KEY_UP:
		{
			CGameView::GetIntanse().lock()->CameraTranslateUp();
		}
		break;
	}
}

void CInput::OnPassiveMouseMove(int x, int y)
{
	static int prevMouseX;
	static int prevMouseY;
	if(glutGetModifiers() == GLUT_ACTIVE_ALT)
	{
		glutSetCursor(GLUT_CURSOR_NONE);
		glutWarpPointer(prevMouseX, prevMouseY);
		CGameView::GetIntanse().lock()->CameraRotate(x - prevMouseX, prevMouseY - y);
		CGameView::GetIntanse().lock()->OnDrawScene();
	}
	else
	{
		glutSetCursor(GLUT_CURSOR_INHERIT);
		prevMouseX = x;
		prevMouseY = y;
	}
}

void CInput::OnMouseMove(int x, int y)
{
	if (m_isLMBDown)
	{
		if(m_ruler)
		{
			CGameView::GetIntanse().lock()->RulerEnd(x, y);
		}
		else
		{
			if(CGameModel::GetIntanse().lock()->GetSelectedObject())
			{
				CGameView::GetIntanse().lock()->TryMoveSelectedObject(x, y);
			}
			else
			{
				//CGameView::GetIntanse().lock()->SelectObjectGroup(startX, startY, x, y);
			}
		}
	}
}