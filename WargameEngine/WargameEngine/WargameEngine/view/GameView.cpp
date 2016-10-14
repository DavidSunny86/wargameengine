#include "GameView.h"
#include <string>
#include "Matrix4.h"
#include "../controller/GameController.h"
#include "../model/ObjectGroup.h"
#include "../LogWriter.h"
#include "../ThreadPool.h"
#include "../Module.h"
#include "../Ruler.h"
#include "CameraStrategy.h"
#include "../UI/UIElement.h"
#include "../UI/UITheme.h"
#include "IImageReader.h"
#include "IModelReader.h"
#include "../Utils.h"
#include "Viewport.h"
#include "OffscreenViewport.h"
#include "CameraFirstPerson.h"
#include "FixedCamera.h"
#include "MirrorCamera.h"
#include "../IPhysicsEngine.h"

using namespace std;
using namespace placeholders;

static const string g_controllerTag = "controller";

CGameView::~CGameView()
{
	m_viewports.clear();
}

CGameView::CGameView(sGameViewContext * context)
	: m_window(move(context->window))
	, m_renderer(&m_window->GetRenderer())
	, m_viewHelper(&m_window->GetViewHelper())
	, m_gameModel(make_unique<CGameModel>())
	, m_shaderManager(m_renderer->CreateShaderManager())
	, m_soundPlayer(move(context->soundPlayer))
	, m_textWriter(move(context->textWriter))
	, m_physicsEngine(move(context->physicsEngine))
	, m_asyncFileProvider(m_threadPool, context->workingDir)
	, m_modelManager(*m_renderer, *m_physicsEngine, m_asyncFileProvider)
	, m_textureManager(m_window->GetViewHelper(), m_asyncFileProvider)
	, m_particles(*m_renderer)
	, m_scriptHandlerFactory(context->scriptHandlerFactory)
	, m_socketFactory(context->socketFactory)
{
	m_viewHelper->SetTextureManager(m_textureManager);
	for (auto& reader : context->imageReaders)
	{
		m_textureManager.RegisterImageReader(std::move(reader));
	}
	for (auto& reader : context->modelReaders)
	{
		m_modelManager.RegisterModelReader(std::move(reader));
	}
	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "english");

	m_ui = make_unique<CUIElement>(*m_renderer, *m_textWriter);
	m_ui->SetTheme(make_shared<CUITheme>(CUITheme::defaultTheme));

	Init(context->module);

	m_window->DoOnDrawScene([this]() {
		m_viewHelper->ClearBuffers(true, true);
		Update();
	});
	m_window->DoOnResize([this](int width, int height) {
		m_ui->Resize(height, width);
		for (auto& viewport : m_viewports)
		{
			viewport->Resize(width, height);
		}
	});
	m_window->DoOnShutdown([this] {
		m_threadPool.CancelAll();
	});

#ifdef _DEBUG
	//m_physicsEngine->EnableDebugDraw(*m_renderer);
#endif

	m_window->LaunchMainLoop();
}

void CGameView::Init(sModule const& module)
{
	m_ui->ClearChildren();
	m_vertexLightning = false;
	m_shadowMapViewport = nullptr;
	m_viewports.clear();
	m_viewports.push_back(std::make_unique<CViewport>(0, 0, 600, 600, 60.0f, *m_viewHelper, true));
	m_viewports.front()->SetCamera(make_unique<CCameraStrategy>(100.0, 100.0, 2.8, 0.5));
	m_gameController.reset();

	m_asyncFileProvider.SetModule(module);
		
	m_gameModel = make_unique<CGameModel>();
	ClearResources();
	InitLandscape();
	InitInput();
	m_gameController = make_unique<CGameController>(*m_gameModel, m_scriptHandlerFactory(), *m_physicsEngine);
	m_gameController->Init(*this, m_socketFactory, m_asyncFileProvider.GetScriptAbsolutePath(module.script));
	m_soundPlayer->Init();
}

void CGameView::InitLandscape()
{
	m_gameModel->GetLandscape().DoOnUpdated([this]() {
		m_tableList.reset();
		m_tableListShadow.reset();
	});
}

void CGameView::WindowCoordsToWorldCoords(int windowX, int windowY, double & worldX, double & worldY, double worldZ)
{
	CVector3d start, end;
	WindowCoordsToWorldVector(windowX, windowY, start, end);
	double a = (worldZ - start.z) / (end.z - start.z);
	worldX = a * (end.x - start.x) + start.x;
	worldY = a * (end.y - start.y) + start.y;
}

void CGameView::WindowCoordsToWorldVector(int x, int y, CVector3d & start, CVector3d & end)
{
	for (auto& viewport : m_viewports)
	{
		if (viewport->PointIsInViewport(x, y))
		{
			m_viewHelper->WindowCoordsToWorldVector(*viewport, x, y, start, end);
			return;
		}
	}
}

void CGameView::InitInput()
{
	m_input = &m_window->ResetInput();
	m_viewports.front()->GetCamera().SetInput(*m_input);
	//UI
	m_input->DoOnLMBDown([this](int x, int y) {
		return m_ui->LeftMouseButtonDown(x, y);
	}, 0);
	m_input->DoOnLMBUp([this](int x, int y) {
		return m_ui->LeftMouseButtonUp(x, y);
	}, 0);
	m_input->DoOnCharacter([this](wchar_t key) {
		return m_ui->OnCharacterInput(key);
	}, 0);
	m_input->DoOnKeyDown([this](int key, int modifiers) {
		return m_ui->OnKeyPress(m_input->KeycodeToVirtualKey(key), modifiers);
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
		if (m_ruler.IsEnabled())
		{
			m_ruler.SetEnd(wx, wy);
		}
		return false;
	}, 2);
	//Game Controller
	m_input->DoOnLMBDown([this](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin, end);
		bool result = m_gameController->OnLeftMouseDown(begin, end, m_input->GetModifiers());
		auto object = m_gameModel->GetSelectedObject();
		if (result && object)
		{
			m_ruler.SetBegin(object->GetX(), object->GetY());
		}
		return result;
	}, 5, g_controllerTag);
	m_input->DoOnLMBUp([this](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin, end);
		bool result = m_gameController->OnLeftMouseUp(begin, end, m_input->GetModifiers());
		if (result && !m_ruler.IsEnabled())
		{
			m_ruler.Hide();
		}
		m_ruler.Disable();
		return result;
	}, 5, g_controllerTag);
	m_input->DoOnMouseMove([this](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin, end);
		bool result = m_gameController->OnMouseMove(begin, end, m_input->GetModifiers());
		auto object = m_gameModel->GetSelectedObject();
		if (result && object)
		{
			m_ruler.SetEnd(object->GetX(), object->GetY());
		}
		return result;
	}, 5, g_controllerTag);
	m_input->DoOnRMBDown([this](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin, end);
		return m_gameController->OnRightMouseDown(begin, end, m_input->GetModifiers());
	}, 5, g_controllerTag);
	m_input->DoOnRMBUp([this](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin, end);
		return m_gameController->OnRightMouseUp(begin, end, m_input->GetModifiers());
	}, 5, g_controllerTag);
	m_input->DoOnGamepadButtonStateChange([this](int gamepadIndex, int buttonIndex, bool newState){
		return m_gameController->OnGamepadButtonStateChange(gamepadIndex, buttonIndex, newState);
	}, 5, g_controllerTag);
	m_input->DoOnGamepadAxisChange([this](int gamepadIndex, int axisIndex, double horizontal, double vertical) {
		return m_gameController->OnGamepadAxisChange(gamepadIndex, axisIndex, horizontal, vertical);
	}, 5, g_controllerTag);
}

void CGameView::DrawUI()
{
	m_viewHelper->DrawIn2D([this] {
		m_ui->Draw();
	});
}

void DrawBBox(sBounding::sBox bbox, double x, double y, double z, double rotation, IRenderer & renderer)
{
	renderer.PushMatrix();
	renderer.Translate(x, y, z);
	renderer.Rotate(rotation, 0.0, 0.0, 1.0);
	renderer.SetColor(0.0f, 0.0f, 255.0f);
	const double * min = bbox.max;
	const double * max = bbox.min;
	renderer.RenderArrays(RenderMode::LINE_LOOP, { CVector3d(min[0], min[1], min[2]), { min[0], max[1], min[2] }, { min[0], max[1], max[2] }, { min[0], min[1], max[2] } }, {}, {});//Left
	renderer.RenderArrays(RenderMode::LINE_LOOP, { CVector3d(min[0], min[1], min[2]), { min[0], min[1], max[2] }, { max[0], min[1], max[2] }, { max[0], min[1], min[2] } }, {}, {});//Back
	renderer.RenderArrays(RenderMode::LINE_LOOP, { CVector3d(max[0], min[1], min[2]), { max[0], max[1], min[2] }, { max[0], max[1], max[2] }, { max[0], min[1], max[2] } }, {}, {});//Right
	renderer.RenderArrays(RenderMode::LINE_LOOP, { CVector3d(min[0], max[1], min[2]), { min[0], max[1], max[2] }, { max[0], max[1], max[2] }, { max[0], max[1], min[2] } }, {}, {}); //Front
	renderer.SetColor(255.0f, 255.0f, 255.0f);
	renderer.PopMatrix();
}

void CGameView::DrawBoundingBox()
{
	shared_ptr<IObject> object = m_gameModel->GetSelectedObject();
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
					auto bbox = m_physicsEngine->GetBounding(object->GetPathToModel());
					DrawBBox(bbox.GetBox(), object->GetX(), object->GetY(), object->GetZ(), object->GetRotation(), *m_renderer);
				}
			}
		}
		else
		{
			auto bbox = m_physicsEngine->GetBounding(object->GetPathToModel());
			DrawBBox(bbox.GetBox(), object->GetX(), object->GetY(), object->GetZ(), object->GetRotation(), *m_renderer);
		}
	}
}

void CGameView::Update()
{
	m_threadPool.Update();
	m_gameController->Update();
	auto& defaultCamera = m_viewports.front()->GetCamera();
	m_soundPlayer->SetListenerPosition(defaultCamera.GetPosition(), defaultCamera.GetDirection());
	m_soundPlayer->Update();
	for (auto it = m_viewports.rbegin();  it != m_viewports.rend(); ++it)
	{
		m_currentViewport = it->get();
		m_currentViewport->Draw([this](bool depthOnly, bool drawUI) {
			if (m_skybox && !depthOnly)
			{
				auto& camera = m_currentViewport->GetCamera();
				CVector3d direction = camera.GetDirection();
				m_skybox->Draw(-direction[0], -direction[1], -direction[2], camera.GetScale());
			}
			DrawObjects(depthOnly);
			if (!depthOnly)
			{
				DrawBoundingBox();
				DrawRuler();
				m_physicsEngine->Draw();
			}
			m_physicsEngine->Update(100 / 60);
			if (drawUI)
			{
				DrawUI();
			}
		});
	}
	m_currentViewport = nullptr;
}

void CGameView::DrawRuler()
{
	if (m_ruler.IsVisible())
	{
		m_renderer->SetColor(255.0f, 255.0f, 0.0f);
		m_renderer->RenderArrays(RenderMode::LINES, { m_ruler.GetBegin(),m_ruler.GetEnd() }, {}, {});
		m_renderer->SetColor(255.0f, 255.0f, 255.0f);
		DrawText3D(m_ruler.GetEnd(), ToWstring(m_ruler.GetDistance(), 2));
	}
}
void CGameView::DrawTable(bool shadowOnly)
{	
	auto& tableList = shadowOnly ? m_tableListShadow : m_tableList;
	tableList = m_renderer->CreateDrawingList([this, shadowOnly] {
		CLandscape const& landscape = m_gameModel->GetLandscape();
		double x1 = -landscape.GetWidth() / 2.0;
		double x2 = landscape.GetWidth() / 2.0;
		double y1 = -landscape.GetDepth() / 2.0;
		double y2 = landscape.GetDepth() / 2.0;
		double xstep = landscape.GetWidth() / (landscape.GetPointsPerWidth() - 1);
		double ystep = landscape.GetDepth() / (landscape.GetPointsPerDepth() - 1);
		m_renderer->SetTexture(landscape.GetTexture());
		unsigned int k = 0;
		for (double x = x1; x <= x2 - xstep; x += xstep)
		{
			vector<CVector3d> vertex;
			vector<CVector2d> texCoord;
			for (double y = y1; y <= y2; y += ystep, k++)
			{
				texCoord.push_back({ (x + x2) / landscape.GetHorizontalTextureScale(), (y + y2) / landscape.GetVerticalTextureScale() });
				vertex.push_back({ x, y, landscape.GetHeight(k) });
				texCoord.push_back({ (x + x2 + xstep) / landscape.GetHorizontalTextureScale(), (y + y2) / landscape.GetVerticalTextureScale() });
				vertex.push_back({ x + xstep, y, landscape.GetHeight(k + 1) });
			}
			m_renderer->RenderArrays(RenderMode::TRIANGLE_STRIP, vertex, {}, texCoord);
		}
		m_renderer->SetTexture(L"");
		if (!shadowOnly)//Don't draw decals because they don't cast shadows
		{
			for (size_t i = 0; i < landscape.GetNumberOfDecals(); ++i)
			{
				sDecal const& decal = landscape.GetDecal(i);
				m_renderer->SetTexture(decal.texture);
				m_renderer->PushMatrix();
				m_renderer->Translate(decal.x, decal.y, 0.0);
				m_renderer->Rotate(decal.rotation, 0.0, 0.0, 1.0);
				m_renderer->RenderArrays(RenderMode::TRIANGLE_STRIP, {
					CVector3d(-decal.width / 2, -decal.depth / 2, landscape.GetHeight(decal.x - decal.width / 2, decal.y - decal.depth / 2) + 0.0001),
					{ -decal.width / 2, decal.depth / 2, landscape.GetHeight(decal.x - decal.width / 2, decal.y + decal.depth / 2) + 0.0001 },
					{ decal.width / 2, -decal.depth / 2, landscape.GetHeight(decal.x + decal.width / 2, decal.y - decal.depth / 2) + 0.0001 },
					{ decal.width / 2, decal.depth / 2, landscape.GetHeight(decal.x + decal.width / 2, decal.y + decal.depth / 2) + 0.0001 }
					}, {},{ CVector2d(0.0, 0.0), { 0.0, 1.0 }, { 1.0, 0.0 }, { 1.0, 1.0 } });
				m_renderer->PopMatrix();
			}
		}
		m_renderer->SetTexture(L"");
	});
}

void CGameView::DrawObjects(bool shadowOnly)
{
	m_viewHelper->EnableDepthTest(true);
	m_viewHelper->EnableBlending(!shadowOnly);
	if (!shadowOnly)
	{
		m_shaderManager->BindProgram();
		if (m_vertexLightning)
		{
			m_viewHelper->EnableVertexLightning(true);
		}
		if (m_shadowMapViewport) SetUpShadowMapDraw();
	}
	auto& list = shadowOnly ? m_tableListShadow : m_tableList;
	if (!list) DrawTable(shadowOnly);
	list->Draw();
	DrawStaticObjects(shadowOnly);
	size_t countObjects = m_gameModel->GetObjectCount();
	auto shaderManager = shadowOnly ? nullptr : m_shaderManager.get();
	for (size_t i = 0; i < countObjects; i++)
	{
		shared_ptr<IObject> object = m_gameModel->Get3DObject(i);
		m_renderer->PushMatrix();
		m_renderer->Translate(object->GetX(), object->GetY(), object->GetCoords().z);
		m_renderer->Rotate(object->GetRotation(), 0.0, 0.0, 1.0);
		m_modelManager.DrawModel(object->GetPathToModel(), object, shadowOnly, shaderManager);
		size_t secondaryModels = object->GetSecondaryModelsCount();
		for (size_t j = 0; j < secondaryModels; ++j)
		{
			m_modelManager.DrawModel(object->GetSecondaryModel(j), object, shadowOnly, shaderManager);
		}
		m_renderer->PopMatrix();
	}
	m_shaderManager->UnBindProgram();
	m_viewHelper->EnableVertexLightning(false);
	if (!shadowOnly)
	{
		for (size_t i = 0; i < m_gameModel->GetProjectileCount(); i++)
		{
			CProjectile const& projectile = m_gameModel->GetProjectile(i);
			m_renderer->PushMatrix();
			m_renderer->Translate(projectile.GetX(), projectile.GetY(), projectile.GetZ());
			m_renderer->Rotate(projectile.GetRotation(), 0.0, 0.0, 1.0);
			if (!projectile.GetPathToModel().empty())
				m_modelManager.DrawModel(projectile.GetPathToModel(), nullptr, false, m_shaderManager.get());
			if (projectile.GetParticle())
				m_particles.Draw(*projectile.GetParticle());
			m_renderer->PopMatrix();
		}
		for (size_t i = 0; i < m_gameModel->GetParticleCount(); ++i)
		{
			CParticleEffect const& effect = m_gameModel->GetParticleEffect(i);
			m_particles.Draw(effect);
		}
	}
	m_viewHelper->EnableDepthTest(false);
}

void CGameView::DrawStaticObjects(bool shadowOnly)
{
	auto& landscape = m_gameModel->GetLandscape();
	for (size_t i = 0; i < landscape.GetStaticObjectCount(); i++)
	{
		CStaticObject const& object = landscape.GetStaticObject(i);
		if (!shadowOnly || object.CastsShadow())
		{
			m_renderer->PushMatrix();
			m_renderer->Translate(object.GetX(), object.GetY(), object.GetZ());
			m_renderer->Rotate(object.GetRotation(), 0.0, 0.0, 1.0);
			m_modelManager.DrawModel(object.GetPathToModel(), nullptr, shadowOnly, m_shaderManager.get());
			m_renderer->PopMatrix();
		}
	}
}

void CGameView::SetUpShadowMapDraw()
{
	Matrix4F cameraModelViewMatrix;
	m_renderer->GetViewMatrix(cameraModelViewMatrix);

	Matrix4F cameraInverseModelViewMatrix = cameraModelViewMatrix.Invert();

	Matrix4F lightMatrix;
	lightMatrix.Scale(0.5f);
	lightMatrix.Translate(0.5, 0.5, 0.5);
	lightMatrix *= m_shadowMapViewport->GetProjectionMatrix();
	lightMatrix *= m_shadowMapViewport->GetViewMatrix();
	lightMatrix *= cameraInverseModelViewMatrix;

	m_shaderManager->SetUniformMatrix4("lightMatrix", 1, lightMatrix);
}

void CGameView::CreateSkybox(double size, wstring const& textureFolder)
{
	m_skybox.reset(new CSkyBox(size, size, size, textureFolder, *m_renderer));
}

CModelManager& CGameView::GetModelManager()
{
	return m_modelManager;
}

IUIElement * CGameView::GetUI() const
{
	return m_ui.get();
}

ISoundPlayer& CGameView::GetSoundPlayer()
{
	return *m_soundPlayer;
}

CTranslationManager& CGameView::GetTranslationManager()
{
	return m_translationManager;
}

CRuler& CGameView::GetRuler()
{
	return m_ruler;
}

IRenderer& CGameView::GetRenderer()
{
	return *m_renderer;
}

void CGameView::ResizeWindow(int height, int width)
{
	m_window->ResizeWindow(width, height);
}

void CGameView::EnableVertexLightning(bool enable)
{ 
	m_vertexLightning = enable;
}

void CGameView::EnableShadowMap(int size, float angle)
{
	if (m_shadowMapViewport) return;
	m_viewports.push_back(std::make_unique<COffscreenViewport>(CachedTextureType::DEPTH, size, size, angle, *m_viewHelper, static_cast<int>(TextureSlot::eShadowMap)));
	m_shadowMapViewport = m_viewports.back().get();
	m_shadowMapViewport->SetCamera(std::make_unique<CFixedCamera>(m_lightPosition, CVector3d({ 0.0, 0.0, 0.0 }), CVector3d({ 0.0, 1.0, 0.0 })));
	m_shadowMapViewport->SetPolygonOffset(true, 2.0f, 500.0f);
	m_shadowMapViewport->SetClippingPlanes(3.0, 300.0);
}

void CGameView::DisableShadowMap()
{
	for (auto it = m_viewports.begin(); it != m_viewports.end(); ++it)
	{
		if (it->get() == m_shadowMapViewport)
		{
			m_viewports.erase(it);
			break;
		}
	}
	m_shadowMapViewport = nullptr;
}

void CGameView::SetLightPosition(int index, float* pos)
{
	m_viewHelper->SetLightPosition(index, pos);
	if (index == 0)
	{
		m_lightPosition = { pos[0], pos[1], pos[2] };
		if(m_shadowMapViewport) m_shadowMapViewport->SetCamera(std::make_unique<CFixedCamera>(m_lightPosition, CVector3d({ 0.0, 0.0, 0.0 }), CVector3d({ 0.0, 0.0, 1.0 })));
	}
}

void CGameView::EnableMSAA(bool enable, int level)
{
	m_window->EnableMultisampling(enable, level);
}

float CGameView::GetMaxAnisotropy() const
{
	return m_viewHelper->GetMaximumAnisotropyLevel();
}

void CGameView::SetAnisotropyLevel(float level)
{
	m_textureManager.SetAnisotropyLevel(level);
}

void CGameView::ClearResources()
{
	m_modelManager.Reset();
	m_textureManager.Reset();
	if (m_skybox)
	{
		m_skybox->ResetList();
	}
	m_tableList.reset();
	m_tableListShadow.reset();
}

void CGameView::SetWindowTitle(wstring const& title)
{
	m_window->SetTitle(title + L" - Wargame Engine");
}

IShaderManager& CGameView::GetShaderManager()
{
	return *m_shaderManager;
}

CAsyncFileProvider& CGameView::GetAsyncFileProvider()
{
	return m_asyncFileProvider;
}

ThreadPool& CGameView::GetThreadPool()
{
	return m_threadPool;
}

IViewHelper& CGameView::GetViewHelper()
{
	return *m_viewHelper;
}

IInput& CGameView::GetInput()
{
	return *m_input;
}

CParticleSystem& CGameView::GetParticleSystem()
{
	return m_particles;
}

size_t CGameView::GetViewportCount() const
{
	return m_viewports.size();
}

IViewport& CGameView::GetViewport(size_t index /*= 0*/)
{
	return *m_viewports.at(index);
}

IViewport& CGameView::AddViewport(std::unique_ptr<IViewport> && viewport)
{
	m_viewports.push_back(std::move(viewport));
	return *m_viewports.back();
}

void CGameView::RemoveViewport(IViewport * viewportPtr)
{
	for (auto it = m_viewports.begin(); it != m_viewports.end(); ++it)
	{
		if (it->get() == viewportPtr)
		{
			m_viewports.erase(it);
			return;
		}
	}
}

void CGameView::Preload(wstring const& image)
{
	if (!image.empty())
	{
		m_viewHelper->ClearBuffers(true, true);
		m_viewHelper->DrawIn2D([this, &image] {
			m_renderer->SetTexture(image);
			float width = 640.0f;//glutGet(GLUT_WINDOW_WIDTH);
			float height = 480.0f;//glutGet(GLUT_WINDOW_HEIGHT);
			m_renderer->RenderArrays(RenderMode::TRIANGLE_STRIP, { CVector2f(0.0f, 0.0f), { 0.0f, height }, { width, 0.0f }, { width, height } }, { CVector2f(0.0f, 0.0f), { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } });
			//glutSwapBuffers();
		});
	}
	size_t countObjects = m_gameModel->GetObjectCount();
	for (size_t i = 0; i < countObjects; i++)
	{
		shared_ptr<const IObject> object = m_gameModel->Get3DObject(i);
		m_modelManager.LoadIfNotExist(object->GetPathToModel());
	}
	m_renderer->SetTexture(L"");
}

void CGameView::LoadModule(wstring const& modulePath)
{
	m_threadPool.CancelAll();
	m_threadPool.QueueCallback([this, modulePath]() {
		sModule module;
		module.Load(modulePath);
		Init(module);
	});
}

void CGameView::DrawText3D(CVector3d const& pos, wstring const& text)
{
	m_viewHelper->DrawIn2D([&] {
		int x, y;
		m_viewHelper->WorldCoordsToWindowCoords(*m_currentViewport, pos, x, y);
		m_textWriter->PrintText(x, y, "times.ttf", 24, text);
	});
}

void CGameView::EnableLight(size_t index, bool enable)
{
	m_viewHelper->EnableLight(index, enable);
}

void CGameView::SetLightColor(size_t index, LightningType type, float * values)
{
	m_viewHelper->SetLightColor(index, type, values);
}

bool CGameView::EnableVRMode(bool enable, bool mirrorToScreen)
{
	ICamera * camera;
	auto viewportFactory = [this, &camera](unsigned int width, unsigned int height) {
		auto& first = AddViewport(std::make_unique<COffscreenViewport>(CachedTextureType::RGBA, width, height, 65.0f, *m_viewHelper));
		auto& second = AddViewport(std::make_unique<COffscreenViewport>(CachedTextureType::RGBA, width, height, 65.0f, *m_viewHelper));
		auto cameraPtr = std::make_unique<CCameraFirstPerson>();
		camera = cameraPtr.get();
		cameraPtr->AttachVR(*m_input);
		first.SetCamera(std::move(cameraPtr));
		second.SetCamera(std::make_unique<CCameraMirror>(camera, CVector3d{0, 0, 0}));
		return std::pair<IViewport&, IViewport&>(first, second);
	};
	bool result = m_window->EnableVRMode(enable, viewportFactory);
	if (enable && result)
	{
		if (mirrorToScreen)
		{
			m_viewports.front()->SetCamera(std::make_unique<CCameraMirror>(camera));
		}
		else
		{
			m_viewports.erase(m_viewports.begin());//remove first viewport
		}
	}
	else
	{
		//restore viewports
	}
	return result;
}

void CGameView::AddParticleEffect(std::wstring const& effectPath, CVector3f const& position, float scale, size_t maxParticles /*= 1000u*/)
{
	m_gameModel->AddParticleEffect(m_particles.GetParticleUpdater(effectPath), effectPath, position, scale, maxParticles);
}

void CGameView::EnableGPUSkinning(bool enable)
{
	m_modelManager.EnableGPUSkinning(enable);
}

sGameViewContext::~sGameViewContext()
{
}
