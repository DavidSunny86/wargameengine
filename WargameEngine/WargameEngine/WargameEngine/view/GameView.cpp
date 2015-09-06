#include "GameView.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <string>
#include <cstring>
#include "MathUtils.h"
#include "../controller/GameController.h"
#include "../model/ObjectGroup.h"
#include "../LogWriter.h"
#include "../ThreadPool.h"
#include "../Module.h"
#include "../Ruler.h"
#include "../SoundPlayerOpenAl.h"
#include "../OSSpecific.h"
#include "CameraStrategy.h"
#include "../UI/UIElement.h"

std::shared_ptr<CGameView> CGameView::m_instanse = NULL;
bool CGameView::m_visible = true;

std::weak_ptr<CGameView> CGameView::GetInstance()
{
	if (!m_instanse.get())
	{
		m_instanse.reset(new CGameView());
		m_instanse->Init();
	}
	std::weak_ptr<CGameView> pView(m_instanse);

	return pView;
}

void CGameView::FreeInstance()
{
	m_instanse.reset();
}

CGameView::~CGameView()
{
	ThreadPool::CancelAll();
	DisableShadowMap();
	CTextureManager::FreeInstance();
	CGameModel::FreeInstance();
}

CGameView::CGameView(void)
	: m_textWriter(m_renderer)
	, m_particles(m_renderer)
	, m_gameModel(CGameModel::GetInstance())
{
	m_ui = std::make_unique<CUIElement>(m_renderer);
	m_ui->SetTheme(std::make_shared<CUITheme>(CUITheme::defaultTheme));
	m_soundPlayer.Init();
}

void CGameView::OnTimer(int value)
{
	if(m_visible) glutPostRedisplay();
	glutTimerFunc(1, OnTimer, value);
}

void CGameView::OnChangeState(int state)
{
	CGameView::m_visible = (state == GLUT_VISIBLE);
}

void CGameView::Init()
{
	setlocale(LC_ALL, ""); 
	setlocale(LC_NUMERIC, "english");
	int argc = 0;
	char* argv[] = {""};
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowSize(600, 600);
	glutCreateWindow("WargameEngine");
	glDepthFunc(GL_LESS);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.01f);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glutDisplayFunc(CGameView::OnDrawScene);
	glutTimerFunc(1, OnTimer, 0);
	glutReshapeFunc(&OnReshape);
	glutKeyboardFunc(&CInput::OnKeyboard);
	glutKeyboardUpFunc(&CInput::OnKeyboardUp);
	glutSpecialFunc(&CInput::OnSpecialKeyPress);
	glutSpecialUpFunc(&CInput::OnSpecialKeyRelease);
	glutMouseFunc(&CInput::OnMouse);
	glutMotionFunc(&CInput::OnMouseMove);
	glutPassiveMotionFunc(&CInput::OnPassiveMouseMove);
	glutMotionFunc(&CInput::OnMouseMove);
	glutCloseFunc(&CGameView::FreeInstance);
	glutWindowStatusFunc(OnChangeState);

	glewInit();
	m_vertexLightning = false;
	m_shadowMap = false;
	memset(m_lightPosition, 0, sizeof(float)* 3);
	m_anisoptropy = 1.0f;
	m_gpuSkinning = false;
	m_camera = std::make_unique<CCameraStrategy>(0.0, 0.0, 2.8, 0.5);
	m_tableList = 0;
	m_tableListShadow = 0;

	m_gameController = std::make_unique<CGameController>(*m_gameModel.lock());
	m_gameController->Init();

	InitInput();

	glutMainLoop();
}

void WindowCoordsToWorldVector(int x, int y, double & startx, double & starty, double & startz, double & endx, double & endy, double & endz)
{
	//Get model, projection and viewport matrices
	double matModelView[16], matProjection[16];
	int viewport[4];
	glGetDoublev(GL_MODELVIEW_MATRIX, matModelView);
	glGetDoublev(GL_PROJECTION_MATRIX, matProjection);
	glGetIntegerv(GL_VIEWPORT, viewport);
	//Set OpenGL Windows coordinates
	double winX = (double)x;
	double winY = viewport[3] - (double)y;

	//Cast a ray from eye to mouse cursor;
	gluUnProject(winX, winY, 0.0, matModelView, matProjection,
		viewport, &startx, &starty, &startz);
	gluUnProject(winX, winY, 1.0, matModelView, matProjection,
		viewport, &endx, &endy, &endz);
}

void WindowCoordsToWorldCoords(int windowX, int windowY, double & worldX, double & worldY, double worldZ = 0)
{
	double startx, starty, startz, endx, endy, endz;
	WindowCoordsToWorldVector(windowX, windowY, startx, starty, startz, endx, endy, endz);
	double a = (worldZ - startz) / (endz - startz);
	worldX = a * (endx - startx) + startx;
	worldY = a * (endy - starty) + starty;
}

static const std::string g_controllerTag = "controller";

void CGameView::InitInput()
{
	m_input = std::make_unique<CInput>();
	m_camera->SetInput(*m_input);
	//UI
	m_input->DoOnLMBDown([this](int x, int y) {
		return m_ui->LeftMouseButtonDown(x, y);
	}, 0);
	m_input->DoOnLMBUp([this](int x, int y) {
		return m_ui->LeftMouseButtonUp(x, y);
	}, 0);
	m_input->DoOnCharacter([this](unsigned int key) {
		return m_ui->OnCharacterInput(key);
	}, 0);
	m_input->DoOnKeyDown([this](int key, int modifiers) {
		return m_ui->OnKeyPress(key, modifiers);
	}, 0);
	m_input->DoOnMouseMove([this](int x, int y) {
		m_ui->OnMouseMove(x, y);
		return false;
	}, 9);
	//Ruler
	m_input->DoOnLMBDown([this](int x, int y) {
		double wx, wy;
		WindowCoordsToWorldCoords(x, y, wx, wy);
		if (m_ruler.IsVisible())
		{
			m_ruler.Hide();
		}
		else
		{
			if (m_ruler.IsEnabled())
			{
				m_ruler.SetBegin(wx, wy);
				return true;
			}
		}
		return false;
	}, 2);
	m_input->DoOnLMBUp([this](int x, int y) {
		double wx, wy;
		WindowCoordsToWorldCoords(x, y, wx, wy);
		m_ruler.SetEnd(wx, wy);
		return false;
	}, 2);
	m_input->DoOnRMBDown([this](int, int) {
		if (m_ruler.IsVisible())
		{
			m_ruler.Hide();
		}
		return false;
	}, 2);
	m_input->DoOnMouseMove([this](int x, int y) {
		double wx, wy;
		WindowCoordsToWorldCoords(x, y, wx, wy);
		m_ruler.SetEnd(wx, wy);
		return false;
	}, 2);
	//Game Controller
	m_input->DoOnLMBDown([this](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin.x, begin.y, begin.z, end.x, end.y, end.z);
		bool result = m_gameController->OnLeftMouseDown(begin, end, m_input->GetModifiers());
		auto object = m_gameModel.lock()->GetSelectedObject();
		if (result && object)
		{
			m_ruler.SetBegin(object->GetX(), object->GetY());
		}
		return result;
	}, 5, g_controllerTag);
	m_input->DoOnLMBUp([this](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin.x, begin.y, begin.z, end.x, end.y, end.z);
		bool result = m_gameController->OnLeftMouseUp(begin, end, m_input->GetModifiers());
		if (result && !m_ruler.IsEnabled())
		{
			m_ruler.Hide();
		}
		return result;
	}, 5, g_controllerTag);
	m_input->DoOnMouseMove([this](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin.x, begin.y, begin.z, end.x, end.y, end.z);
		bool result = m_gameController->OnMouseMove(begin, end, m_input->GetModifiers());
		auto object = m_gameModel.lock()->GetSelectedObject();
		if (result && object)
		{
			m_ruler.SetEnd(object->GetX(), object->GetY());
		}
		return result;
	}, 5, g_controllerTag);
	m_input->DoOnRMBDown([this](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin.x, begin.y, begin.z, end.x, end.y, end.z);
		return m_gameController->OnRightMouseDown(begin, end, m_input->GetModifiers());
	}, 5, g_controllerTag);
	m_input->DoOnRMBUp([this](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin.x, begin.y, begin.z, end.x, end.y, end.z);
		return m_gameController->OnRightMouseUp(begin, end, m_input->GetModifiers());
	}, 5, g_controllerTag);
}

void CGameView::OnDrawScene()
{
	CGameView::GetInstance().lock()->DrawShadowMap();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CGameView::GetInstance().lock()->Update();
	glutSwapBuffers();
}

void CGameView::DrawUI()
{
	glEnable(GL_BLEND);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0,glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT),0,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	m_ui->Draw();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glDisable(GL_BLEND);
}

void DrawBBox(IBounding* ibox, double x, double y, double z, double rotation)
{
	if (dynamic_cast<CBoundingCompound*>(ibox) != NULL)
	{
		CBoundingCompound * bbox = (CBoundingCompound *)ibox;
		for (size_t i = 0; i < bbox->GetChildCount(); ++i)
		{
			DrawBBox(bbox->GetChild(i), x, y, z, rotation);
		}
		return;
	}
	CBoundingBox * bbox = (CBoundingBox *)ibox;
	glPushMatrix();
	glTranslated(x, y, z);
	glRotated(rotation, 0.0, 0.0, 1.0);
	glScaled(bbox->GetScale(), bbox->GetScale(), bbox->GetScale());
	glColor3d(0.0, 0.0, 255.0);
	const double * min = bbox->GetMin();
	const double * max = bbox->GetMax();
	//Left
	glBegin(GL_LINE_LOOP);
	glVertex3d(min[0], min[1], min[2]);
	glVertex3d(min[0], max[1], min[2]);
	glVertex3d(min[0], max[1], max[2]);
	glVertex3d(min[0], min[1], max[2]);
	
	glEnd();
	//Right
	glBegin(GL_LINE_LOOP);
	glVertex3d(max[0], min[1], min[2]);
	glVertex3d(max[0], max[1], min[2]);
	glVertex3d(max[0], max[1], max[2]);
	glVertex3d(max[0], min[1], max[2]);
	glEnd();
	//Front
	glBegin(GL_LINE_LOOP);
	glVertex3d(min[0], max[1], min[2]);
	glVertex3d(min[0], max[1], max[2]);
	glVertex3d(max[0], max[1], max[2]);
	glVertex3d(max[0], max[1], min[2]);
	glEnd();
	//Back
	glBegin(GL_LINE_LOOP);
	glVertex3d(min[0], min[1], min[2]);
	glVertex3d(min[0], min[1], max[2]);
	glVertex3d(max[0], min[1], max[2]);
	glVertex3d(max[0], min[1], min[2]);
	glEnd();
	glColor3d(255.0, 255.0, 255.0);
	glPopMatrix();
}

void CGameView::DrawBoundingBox()
{
	std::shared_ptr<IObject> object = m_gameModel.lock()->GetSelectedObject();
	if(object)
	{
		if (CGameModel::IsGroup(object.get()))
		{
			CObjectGroup * group = (CObjectGroup *)object.get();
			for(size_t i = 0; i < group->GetCount(); ++i)
			{
				object = group->GetChild(i);
				if(object)
				{
					auto bbox = m_gameModel.lock()->GetBoundingBox(object->GetPathToModel());
					if (bbox)
					{
						DrawBBox(bbox.get(), object->GetX(), object->GetY(), object->GetZ(), object->GetRotation());
					}
				}
			}
		}
		else
		{
			auto bbox = m_gameModel.lock()->GetBoundingBox(object->GetPathToModel());
			if(bbox) DrawBBox(bbox.get(), object->GetX(), object->GetY(), object->GetZ(), object->GetRotation());
		}
	}
}

void CGameView::Update()
{
	ThreadPool::Update();
	const double * position = m_camera->GetPosition();
	const double * direction = m_camera->GetDirection();
	const double * up = m_camera->GetUpVector();
	m_soundPlayer.SetListenerPosition(CVector3d(position), CVector3d(direction));
	m_soundPlayer.Update();
	if (m_skybox) m_skybox->Draw(-direction[0], -direction[1], -direction[2], m_camera->GetScale());
	glLoadIdentity();
	gluLookAt(position[0], position[1], position[2], direction[0], direction[1], direction[2], up[0], up[1], up[2]);
	m_gameController->Update();
	DrawObjects();
	DrawBoundingBox();
	m_ruler.Draw();
	DrawUI();
}

void CGameView::ResetTable()
{
	if (m_tableList) glDeleteLists(m_tableList, 1);
	if (m_tableListShadow) glDeleteLists(m_tableListShadow, 1);
	m_tableList = 0;
	m_tableListShadow = 0;
}

void CGameView::DrawTable(bool shadowOnly)
{
	if (shadowOnly)
	{
		m_tableListShadow = glGenLists(1);
		glNewList(m_tableListShadow, GL_COMPILE);
	}
	else
	{
		m_tableList = glGenLists(1);
		glNewList(m_tableList, GL_COMPILE);
	}
	
	CLandscape const& landscape = CGameModel::GetInstance().lock()->GetLandscape();
	double x1 = -landscape.GetWidth() / 2.0;
	double x2 = landscape.GetWidth() / 2.0;
	double y1 = -landscape.GetDepth() / 2.0;
	double y2 = landscape.GetDepth() / 2.0;
	double xstep = landscape.GetWidth() / (landscape.GetPointsPerWidth() - 1);
	double ystep = landscape.GetDepth() / (landscape.GetPointsPerDepth() - 1);
	CTextureManager::GetInstance()->SetTexture(landscape.GetTexture());
	unsigned int k = 0;
	for(double x = x1; x <= x2 - xstep; x += xstep)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(double y = y1; y <= y2; y += ystep, k++)
		{
			glTexCoord2d((x + x2) / landscape.GetHorizontalTextureScale(), (y + y2) / landscape.GetVerticalTextureScale());
			glVertex3d(x, y, landscape.GetHeight(k));
			glTexCoord2d((x + x2 + xstep) / landscape.GetHorizontalTextureScale(), (y + y2) / landscape.GetVerticalTextureScale());
			glVertex3d(x + xstep, y, landscape.GetHeight(k + 1));
		}
		glEnd();
	}
	CTextureManager::GetInstance()->SetTexture("");
	for (size_t i = 0; i < landscape.GetStaticObjectCount(); i++)
	{
		CStaticObject const& object = landscape.GetStaticObject(i);
		glPushMatrix();
		glTranslated(object.GetX(), object.GetY(), 0.0);
		glRotated(object.GetRotation(), 0.0, 0.0, 1.0);
		m_modelManager.DrawModel(object.GetPathToModel(), nullptr, shadowOnly, m_gpuSkinning);
		glPopMatrix();
	}
	if (!shadowOnly)//Down't draw decals because they don't cast shadows
	{
		for (size_t i = 0; i < landscape.GetNumberOfDecals(); ++i)
		{
			sDecal const& decal = landscape.GetDecal(i);
			CTextureManager::GetInstance()->SetTexture(decal.texture);
			glPushMatrix();
			glTranslated(decal.x, decal.y, 0.0);
			glRotated(decal.rotation, 0.0, 0.0, 1.0);
			glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2d(0.0, 0.0);
			glVertex3d(-decal.width / 2, -decal.depth / 2, landscape.GetHeight(decal.x - decal.width / 2, decal.y - decal.depth / 2) + 0.0001);
			glTexCoord2d(0.0, 1.0);
			glVertex3d(-decal.width / 2, decal.depth / 2, landscape.GetHeight(decal.x - decal.width / 2, decal.y + decal.depth / 2) + 0.0001);
			glTexCoord2d(1.0, 0.0);
			glVertex3d(decal.width / 2, -decal.depth / 2, landscape.GetHeight(decal.x + decal.width / 2, decal.y - decal.depth / 2) + 0.0001);
			glTexCoord2d(1.0, 1.0);
			glVertex3d(decal.width / 2, decal.depth / 2, landscape.GetHeight(decal.x + decal.width / 2, decal.y + decal.depth / 2) + 0.0001);
			glEnd();
			glPopMatrix();
		}
	}
	CTextureManager::GetInstance()->SetTexture("");
	glEndList();
}

void CGameView::DrawObjects(void)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	m_shader.BindProgram();
	if (m_vertexLightning)
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glEnable(GL_LIGHTING);
	}
	if (m_shadowMap) SetUpShadowMapDraw();
	if (m_tableList == 0) DrawTable(false);
	else glCallList(m_tableList);
	size_t countObjects = m_gameModel.lock()->GetObjectCount();
	for (size_t i = 0; i < countObjects; i++)
	{
		std::shared_ptr<IObject> object = m_gameModel.lock()->Get3DObject(i);
		glPushMatrix();
		glTranslated(object->GetX(), object->GetY(), 0.0);
		glRotated(object->GetRotation(), 0.0, 0.0, 1.0);
		m_modelManager.DrawModel(object->GetPathToModel(), object, false, m_gpuSkinning);
		size_t secondaryModels = object->GetSecondaryModelsCount();
		for (size_t j = 0; j < secondaryModels; ++j)
		{
			m_modelManager.DrawModel(object->GetSecondaryModel(j), object, false, m_gpuSkinning);
		}
		glPopMatrix();
	}
	m_shader.UnBindProgram();
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	for (size_t i = 0; i < m_gameModel.lock()->GetProjectileCount(); i++)
	{
		CProjectile const& projectile = m_gameModel.lock()->GetProjectile(i);
		glPushMatrix();
		glTranslated(projectile.GetX(), projectile.GetY(), projectile.GetZ());
		glRotated(projectile.GetRotation(), 0.0, 0.0, 1.0);
		if (!projectile.GetPathToModel().empty())
			m_modelManager.DrawModel(projectile.GetPathToModel(), nullptr, false, m_gpuSkinning);
		if (!projectile.GetParticle().empty())
			m_particles.DrawEffect(projectile.GetParticle(), projectile.GetTime());
		glPopMatrix();
	}
	m_particles.DrawParticles();
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glDisable(GL_DEPTH_TEST);
}

void CGameView::SetUpShadowMapDraw()
{
	float cameraModelViewMatrix[16];
	float cameraInverseModelViewMatrix[16];
	float lightMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, cameraModelViewMatrix);
	InvertMatrix(cameraModelViewMatrix, cameraInverseModelViewMatrix);

	glPushMatrix();
	glLoadIdentity();
	glTranslatef(0.5, 0.5, 0.5); // + 0.5
	glScalef(0.5, 0.5, 0.5); // * 0.5
	glMultMatrixf(m_lightProjectionMatrix);
	glMultMatrixf(m_lightModelViewMatrix);
	glMultMatrixf(cameraInverseModelViewMatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, lightMatrix);
	glPopMatrix();

	m_shader.SetUniformMatrix4("lightMatrix", 1, lightMatrix);
}

void CGameView::DrawShadowMap()
{
	if (!m_shadowMap) return;
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0, 500.0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(m_shadowAngle, 1.0, 3.0, 300.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(m_lightPosition[0], m_lightPosition[1], m_lightPosition[2], 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	glGetFloatv(GL_PROJECTION_MATRIX, m_lightProjectionMatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, m_lightModelViewMatrix);

	if (m_tableListShadow == 0) DrawTable(true);
	else glCallList(m_tableListShadow);

	size_t countObjects = m_gameModel.lock()->GetObjectCount();
	for (size_t i = 0; i < countObjects; i++)
	{
		std::shared_ptr<IObject> object = m_gameModel.lock()->Get3DObject(i);
		if (!object->CastsShadow()) continue;
		glPushMatrix();
		glTranslated(object->GetX(), object->GetY(), 0);
		glRotated(object->GetRotation(), 0.0, 0.0, 1.0);
		m_modelManager.DrawModel(object->GetPathToModel(), object, true, m_gpuSkinning);
		size_t secondaryModels = object->GetSecondaryModelsCount();
		for (size_t j = 0; j < secondaryModels; ++j)
		{
			m_modelManager.DrawModel(object->GetSecondaryModel(j), object, true, m_gpuSkinning);
		}
		glPopMatrix();
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
}

void CGameView::OnReshape(int width, int height) 
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLdouble aspect = (GLdouble)width / (GLdouble)height;
	gluPerspective(60, aspect, 0.5, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	CGameView::GetInstance().lock()->m_ui->Resize(height, width);
}

void CGameView::CreateSkybox(double size, std::string const& textureFolder)
{
	m_skybox.reset(new CSkyBox(size, size, size, textureFolder, m_renderer));
}

CGameController& CGameView::GetController()
{
	return *m_gameController;
}

void CGameView::ResetController()
{
	m_input->DeleteAllSignalsByTag(g_controllerTag);
	m_gameController.reset();
	CGameModel::FreeInstance();
	m_gameModel = CGameModel::GetInstance();
	m_gameController = std::make_unique<CGameController>(*m_gameModel.lock());
}

ICamera * CGameView::GetCamera()
{
	return m_camera.get();
}

void CGameView::SetCamera(ICamera * camera)
{
	m_camera.reset(camera);
	m_camera->SetInput(*m_input);
}

CModelManager& CGameView::GetModelManager()
{
	return m_modelManager;
}

IUIElement * CGameView::GetUI() const
{
	return m_ui.get();
}

CParticleSystem& CGameView::GetParticleSystem()
{
	return m_particles;
}

CTextWriter& CGameView::GetTextWriter()
{
	return m_textWriter;
}

ISoundPlayer& CGameView::GetSoundPlayer()
{
	return m_soundPlayer;
}

CTranslationManager& CGameView::GetTranslationManager()
{
	return m_translationManager;
}

CRuler& CGameView::GetRuler()
{
	return m_ruler;
}

void CGameView::ResizeWindow(int height, int width)
{
	glutReshapeWindow(width, height);
}

void CGameView::NewShaderProgram(std::string const& vertex, std::string const& fragment, std::string const& geometry)
{
	m_shader.NewProgram(vertex, fragment, geometry);
}

void CGameView::EnableVertexLightning(bool enable)
{ 
	m_vertexLightning = enable;
	if (enable)
		glEnable(GL_NORMALIZE);
	else
		glDisable(GL_NORMALIZE);
}

void CGameView::EnableShadowMap(int size, float angle)
{
	if (m_shadowMap) return;
	if (!GLEW_ARB_depth_buffer_float)
	{
		LogWriter::WriteLine("GL_ARB_depth_buffer_float is not supported, shadow maps cannot be enabled");
		return;
	}
	if (!GLEW_EXT_framebuffer_object)
	{
		LogWriter::WriteLine("GL_EXT_framebuffer_object is not supported, shadow maps cannot be enabled");
		return;
	}
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &m_shadowMapTexture);
	glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size, size,
		0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE, 0);
	glGenFramebuffers(1, &m_shadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
	glDrawBuffer(GL_NONE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMapTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogWriter::WriteLine("Cannot enable shadowmaps. Error creating framebuffer.");
		glDeleteTextures(1, &m_shadowMapTexture);
		return;
	}
	m_shadowMap = true;
	m_shadowMapSize = size;
	m_shadowAngle = angle;
	
}

void CGameView::DisableShadowMap()
{
	if (!m_shadowMap) return;
	glDeleteTextures(1, &m_shadowMapTexture);
	glDeleteFramebuffersEXT(1, &m_shadowMapFBO);
	m_shadowMap = false;
}

void CGameView::SetLightPosition(int index, float* pos)
{
	glLightfv(GL_LIGHT0 + index, GL_POSITION, pos);
	if(index == 0) memcpy(m_lightPosition, pos, sizeof(float)* 3);
}

void CGameView::EnableMSAA(bool enable)
{
	if (GLEW_ARB_multisample)
	{
		if (enable)
			glEnable(GL_MULTISAMPLE_ARB);
		else
			glDisable(GL_MULTISAMPLE_ARB);
	}
	else
	{
		LogWriter::WriteLine("MSAA is not supported");
	}
}

float CGameView::GetAnisotropyLevel() const
{
	return m_anisoptropy;
}

float CGameView::GetMaxAnisotropy()
{
	float aniso = 1.0f;
	if (GLEW_EXT_texture_filter_anisotropic)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
	return aniso;
}

void CGameView::SetAnisotropy(float maxAnisotropy)
{
	m_anisoptropy = maxAnisotropy;
	CTextureManager::GetInstance()->SetAnisotropyLevel(maxAnisotropy);
}

void CGameView::ClearResources()
{
	CTextureManager::FreeInstance();
	m_modelManager = CModelManager();
	CTextureManager::GetInstance();
	m_skybox->ResetList();
	ResetTable();
}

void CGameView::SetWindowTitle(std::string const& title)
{
	glutSetWindowTitle((title + " - Wargame Engine").c_str());
}

CShaderManager const* CGameView::GetShaderManager() const
{
	return &m_shader;
}

void CGameView::Preload(std::string const& image)
{
	if (!image.empty())
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		CTextureManager::GetInstance()->SetTexture(image);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2d(0.0, 0.0);
		glVertex2d(0.0, 0.0);
		glTexCoord2d(0.0, 1.0);
		glVertex2d(0.0, glutGet(GLUT_WINDOW_HEIGHT));
		glTexCoord2d(1.0, 0.0);
		glVertex2d(glutGet(GLUT_WINDOW_WIDTH), 0.0);
		glTexCoord2d(1.0, 1.0);
		glVertex2d(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
		glutSwapBuffers();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glutSwapBuffers();
	}
	size_t countObjects = m_gameModel.lock()->GetObjectCount();
	for (size_t i = 0; i < countObjects; i++)
	{
		std::shared_ptr<const IObject> object = m_gameModel.lock()->Get3DObject(i);
		m_modelManager.LoadIfNotExist(object->GetPathToModel());
	}
	CTextureManager::GetInstance()->SetTexture("");
}

void CGameView::LoadModuleCallback(int)
{
	auto view = CGameView::GetInstance().lock();
	view->ResetController();
	view->GetUI()->ClearChildren();
	view->GetController().Init();
	view->InitInput();
}

void CGameView::LoadModule(std::string const& module)
{
	ThreadPool::CancelAll();
	sModule::Load(module);
	ChangeWorkingDirectory(sModule::folder);
	CTextureManager::FreeInstance();
	m_modelManager = CModelManager();
	m_skybox.reset();
	m_vertexLightning = false;
	m_shadowMap = false;
	memset(m_lightPosition, 0, sizeof(float) * 3);
	m_anisoptropy = 1.0f;
	glutTimerFunc(1, LoadModuleCallback, 0);
}

void CGameView::ToggleFullscreen() const 
{
	glutFullScreenToggle();
}

void CGameView::DrawLine(double beginX, double beginY, double beginZ, double endX, double endY, double endZ, unsigned char colorR, unsigned char colorG, unsigned char colorB) const
{
	glColor3ub(colorR, colorG, colorB);
	glBegin(GL_LINES);
	glVertex3d(beginX, beginY, beginZ);
	glVertex3d(endX, endY, endZ);
	glEnd();
	glColor3d(255.0, 255.0, 255.0);
}

void CGameView::DrawLineLoop(double * points, unsigned int size, unsigned char colorR, unsigned char colorG, unsigned char colorB) const
{
	glColor3ub(colorR, colorG, colorB);
	glBegin(GL_LINE_LOOP);
	for (unsigned int i = 0; i < size; i += 3)
	{
		glVertex3d(points[i], points[i + 1], points[i + 2]);
	}
	glEnd();
	glColor3d(255.0, 255.0, 255.0);
}

void CGameView::DrawText3D(double x, double y, double z, std::string const& text)
{
	glRasterPos3d(x, y, z); // location to start printing text
	for (size_t i = 0; i < text.size(); i++) // loop until i is greater then l
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, text[i]); // Print a character on the screen
	}
}

void CGameView::EnableLight(size_t index, bool enable)
{
	if (enable)
	{
		glEnable(GL_LIGHT0 + index);
	}
	else
	{
		glDisable(GL_LIGHT0 + index);
	}
}

static const std::map<LightningType, GLenum> lightningTypesMap = {
	{ LightningType::DIFFUSE, GL_DIFFUSE },
	{ LightningType::AMBIENT, GL_AMBIENT },
	{ LightningType::SPECULAR, GL_SPECULAR }
};

void CGameView::SetLightColor(size_t index, LightningType type, float * values)
{
	glLightfv(GL_LIGHT0 + index, lightningTypesMap.at(type), values);
}

void CGameView::EnableGPUSkinning(bool enable)
{
	m_gpuSkinning = enable;
}