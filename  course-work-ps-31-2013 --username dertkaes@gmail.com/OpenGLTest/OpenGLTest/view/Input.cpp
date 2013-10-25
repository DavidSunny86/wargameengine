#include "Input.h"
#include <GL\glut.h>
#include "GameView.h"

bool CInput::m_isLMBDown = false;
bool CInput::m_ruler = false;

void CInput::OnMouse(int button, int state, int x, int y)
{
	switch(button)
	{
	case GLUT_LEFT_BUTTON: //LMB
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
				CGameView::GetIntanse().lock()->SelectObject(x, y);
				CGameView::GetIntanse().lock()->RulerHide();
			}
		}
		else
		{
			m_isLMBDown = false;
			if(CGameView::GetIntanse().lock()->UILeftMouseButtonUp(x, y)) return;
			m_ruler = false;
		}break;
	case 3://scroll up
		if (state == GLUT_UP)
		{
			CGameView::GetIntanse().lock()->CameraZoomIn();
		}break;
	case 4://scroll down
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
	case 8://backspace
		{
			CGameView::GetIntanse().lock()->CameraReset();
		}break;
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
		  }break;
	 case GLUT_KEY_RIGHT:
		  {
			 CGameView::GetIntanse().lock()->CameraTranslateRight();
		  }break;
	 case GLUT_KEY_DOWN:
		  {
			  CGameView::GetIntanse().lock()->CameraTranslateDown();
		  }break;
	 case GLUT_KEY_UP:
		  {
			  CGameView::GetIntanse().lock()->CameraTranslateUp();
		  }break;
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
		CGameView::GetIntanse().lock()->OnDrawScene;
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
			CGameView::GetIntanse().lock()->TryMoveSelectedObject(x, y);
		}
	}
}