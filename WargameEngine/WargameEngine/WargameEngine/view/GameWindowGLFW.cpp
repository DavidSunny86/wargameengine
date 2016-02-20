#include "GameWindowGLFW.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "InputGLFW.h"
#include "OpenGLRenderer.h"

static CGameWindowGLFW* g_instance = nullptr;
bool CGameWindowGLFW::m_visible = true;

void CGameWindowGLFW::OnChangeState(GLFWwindow * /*window*/, int state)
{
	m_visible = (state == GLFW_VISIBLE);
}

void CGameWindowGLFW::OnReshape(GLFWwindow * /*window*/, int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLdouble aspect = (GLdouble)width / (GLdouble)height;
	gluPerspective(60, aspect, 0.5, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	if (g_instance->m_onResize)
	{
		g_instance->m_onResize(width, height);
	}
}

void CGameWindowGLFW::OnShutdown(GLFWwindow * window)
{
	if (g_instance->m_onShutdown)
	{
		g_instance->m_onShutdown;
	}
	glfwDestroyWindow(window);
	g_instance->m_window = nullptr;
}

void CGameWindowGLFW::LaunchMainLoop()
{
	while (m_window && !glfwWindowShouldClose(m_window))
	{
		if (m_visible)
		{
			if (g_instance->m_onDraw)
			{
				g_instance->m_onDraw();
			}
			glfwSwapBuffers(g_instance->m_window);
		}
		glfwPollEvents();
	}
	glfwTerminate();
}

void CGameWindowGLFW::CreateNewWindow(GLFWmonitor * monitor /*= NULL*/)
{
	if (m_window)
	{
		glfwDestroyWindow(m_window);
	}
	m_window = glfwCreateWindow(600, 600, "WargameEngine", monitor, NULL);
	glfwMakeContextCurrent(m_window);
	//glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glDepthFunc(GL_LESS);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.01f);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glfwSetWindowSizeCallback(m_window, &OnReshape);
	glfwSetKeyCallback(m_window, &CInputGLFW::OnKeyboard);
	glfwSetCharCallback(m_window, &CInputGLFW::OnCharacter);
	glfwSetMouseButtonCallback(m_window, &CInputGLFW::OnMouse);
	glfwSetCursorPosCallback(m_window, &CInputGLFW::OnMouseMove);
	glfwSetScrollCallback(m_window, &CInputGLFW::OnScroll);
	glfwSetWindowCloseCallback(m_window, &CGameWindowGLFW::OnShutdown);
	glfwSetWindowIconifyCallback(m_window, &OnChangeState);
}

CGameWindowGLFW::CGameWindowGLFW()
{
	g_instance = this;

	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 16);

	CreateNewWindow();

	m_renderer = std::make_unique<COpenGLRenderer>();

	glewInit();

	//glfwSwapInterval(1);

}

void CGameWindowGLFW::DoOnDrawScene(std::function<void()> const& handler)
{
	m_onDraw = handler;
}

void CGameWindowGLFW::DoOnResize(std::function<void(int, int)> const& handler)
{
	m_onResize = handler;
	int width, height;
	glfwGetWindowSize(m_window, &width, &height);
	OnReshape(m_window, width, height);
}

void CGameWindowGLFW::DoOnShutdown(std::function<void()> const& handler)
{
	m_onShutdown = handler;
}

void CGameWindowGLFW::ResizeWindow(int width, int height)
{
	glfwSetWindowSize(m_window, width, height);
}

void CGameWindowGLFW::SetTitle(std::string const& title)
{
	glfwSetWindowTitle(m_window, title.c_str());
}

void CGameWindowGLFW::ToggleFullscreen()
{
	CreateNewWindow(glfwGetPrimaryMonitor());
}

void CGameWindowGLFW::EnableMultisampling(bool enable, int level /*= 1.0f*/)
{
	if (enable)
		glEnable(GL_MULTISAMPLE);
	else
		glDisable(GL_MULTISAMPLE);
	glfwWindowHint(GLFW_SAMPLES, level);
}

IInput& CGameWindowGLFW::ResetInput()
{
	m_input = std::make_unique<CInputGLFW>(m_window);
	return *m_input;
}

IRenderer& CGameWindowGLFW::GetRenderer()
{
	return *m_renderer;
}

IViewHelper& CGameWindowGLFW::GetViewHelper()
{
	return *m_renderer;
}

