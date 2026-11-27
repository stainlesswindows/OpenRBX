// clang-format off
#include <OgreAnimation.h>
#include <OgreCamera.h>
#include <OgreRenderSystem.h>
#include <OgreRenderTarget.h>
#include <OgreSceneManager.h>
#include <OgreStringConverter.h>
#include <OgreTimer.h>
// clang-format on

#include "RobloxWnd.h"

#include "LogManager.h"
#include "NativeInput.h"
#include "RenderSettings.h"
#include "Roblox.h"
#include "SystemMessageBox.h"
#include "View.h"
#include "Workspace.h"
#include "appdraw/AdornG3D.h"
#include "appdraw/AdornRbxGfx.h"
#include "appdraw/RbxEntity.h"
#include "appdraw/RbxSceneManager.h"
#include "appdraw/RbxSceneNode.h"
#include "appdraw/VisualEngine.h"
#include "network/Players.h"
#include "rbxview/View.h"
#include "reflection/enumconverter.h"
#include "renderlib/Mesh.h"
#include "renderlib/Profiler.h"
#include "renderlib/RenderStats.h"
#include "renderlibnew/Chunk.h"
#include "util/FrameInterpolator.h"
#include "util/RunStateOwner.h"
#include "util/Timer.h"
#include "util/Utilities.h"
#include "util/scopedassign.h"
#include "util/standardout.h"
#include "v8datamodel/Camera.h"
#include "v8datamodel/GlobalSettings.h"
#include "v8datamodel/Stats.h"
#include "v8datamodel/TimeState.h"
#include "v8datamodel/UserController.h"

#include <G3D/Log.h>
#include <G3D/stringutils.h>
#include <GLG3D/DXCaps.h>
#include <GLG3D/GLCaps.h>
#include <GLG3D/Texture.h>
#include <GLG3D/VARArea.h>
#include <QCoreApplication>
#include <algorithm>
#include <cstdio>
#include <set>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <GLG3D/Win32Window.h>
#else
#include "GlxWindow.h"
#include "InputDevice.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QWheelEvent>
#include <qguiapplication_platform.h>
#endif

#ifdef _WIN32
static const CRenderSettings::GraphicsMode rbxGfxGraphicsMode = CRenderSettings::c_graphicsModeDirect3D;
#else
static const CRenderSettings::GraphicsMode rbxGfxGraphicsMode = CRenderSettings::c_graphicsModeOpenGL;
#endif

static std::mutex throttleMutex;

static bool repaintRequested;

static std::set<CRobloxWnd*> idleWindows;

class RenderFrame
{
public:
	RenderFrame(RBX::DataModel* dataModel)
	{
		dataModel->timeState->fpsStopwatch.tick();
		dataModel->timeState->fpsStopwatch.tock();

		if (RBX::RunService* runService = dataModel->find<RBX::RunService>()) {
			RBX::FrameInterpolator::beginRender(
				runService->getStepCount(), runService->getStepTime(), runService->getPeriod()
			);
		}
	}

	~RenderFrame() { RBX::FrameInterpolator::endRender(); }
};

class ScopedStopwatch
{
public:
	ScopedStopwatch(G3D::Stopwatch& stopwatch) : m_stopwatch(&stopwatch) { stopwatch.tick(); }

	~ScopedStopwatch() { m_stopwatch->tock(); }

private:
	G3D::Stopwatch* m_stopwatch;
};

static void runViewValid(RBX::DataModel* dataModel, RBX::IRunView* runView)
{
	if (dataModel) {
		dataModel->create<RBX::RunService>()->runViewValid(runView);
	}
}

static void removeRunView(RBX::DataModel* dataModel, RBX::IRunView* runView)
{
	if (dataModel) {
		dataModel->create<RBX::RunService>()->removeRunView(runView);
	}
}

void CRobloxWnd::resizeRbxGfx(int cx, int cy)
{
	if (m_visualEngine) {
		Ogre::RenderWindow* window = m_visualEngine->m_renderWindow;
		Ogre::Camera* camera = m_visualEngine->m_camera;
#ifndef _WIN32
		Display* display = nullptr;
		unsigned long nativeWindow = 0;
		window->getCustomAttribute("DISPLAY", &display);
		window->getCustomAttribute("WINDOW", &nativeWindow);
		resizeNativeWindow(display, nativeWindow, cx, cy);
#endif
		window->windowMovedOrResized();

		if (camera) {
			if (height() != 0) {
				camera->setAspectRatio(Ogre::Real(window->getWidth()) / Ogre::Real(window->getHeight()));
			}
			camera->yaw(Ogre::Radian(0.0f));
		}
	}
}

void CRobloxWnd::createRbxGfxView()
{
	m_view.reset(new RBX::ViewNew::View(m_dataModel));
	m_adorn.reset(new RBX::AdornRbxGfx(m_visualEngine));
}

void CRobloxWnd::buildGui()
{
	if (m_glWindow) {
		m_glWindow->makeCurrent();
	}
	m_dataModel->buildGui(m_adorn.get());
}

#ifdef _WIN32
bool CRobloxWnd::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
	if (!m_created) {
		return QWidget::nativeEvent(eventType, message, result);
	}

	MSG* msg = static_cast<MSG*>(message);
	HWND hwnd = msg->hwnd;
	bool handled;

	if (msg->message == NativeInput::focusViewportMessage) {
		setFocus();
		*result = 0;
		return true;
	}

	if (m_userInput && m_userInput->isMouseAcquired()) {
		switch (msg->message) {
		case WM_MOUSEWHEEL:
			*result = SendMessageW(GetAncestor(hwnd, GA_ROOT), msg->message, msg->wParam, msg->lParam);
			return true;
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
			*result = 0;
			return true;
		case WM_SETCURSOR:
			SetCursor(nullptr);
			*result = TRUE;
			return true;
		}
	}

	switch (msg->message) {
	case WM_CREATE:
		return false;
	case WM_DESTROY:
		OnDestroy();
		return false;
	case WM_ERASEBKGND:
		*result = TRUE;
		return true;
	case WM_PAINT:
		OnPaint();
		*result = 0;
		return true;
	case WM_SIZE:
		OnSize(LOWORD(msg->lParam), HIWORD(msg->lParam));
		return false;
	case WM_TIMER:
		OnTimer(static_cast<unsigned>(msg->wParam));
		*result = 0;
		return true;
	case WM_WINDOWPOSCHANGING:
	case WM_CLOSE:
		swapBuffers();
		return false;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
		if (m_userInput && msg->message == WM_KEYDOWN) {
			m_userInput->processMessage(msg->message, static_cast<int>(msg->wParam), msg->lParam);
		}
		*result = SendMessageW(GetAncestor(hwnd, GA_ROOT), msg->message, msg->wParam, msg->lParam);
		return true;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		setFocus();
		[[fallthrough]];
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
		handled = true;
		*result = 0;
		break;
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
		handled = false;
		*result = 0;
		break;
	default:
		return false;
	}

	if (m_dataModel) {
		if (m_glWindow) {
			m_glWindow->makeCurrent();
		}
		if (m_userInput) {
			m_userInput->processMessage(msg->message, static_cast<int>(msg->wParam), msg->lParam);
		}
		InvalidateRect(hwnd, nullptr, TRUE);
	}

	return handled;
}
#else
void CRobloxWnd::paintEvent(QPaintEvent* event)
{
	OnPaint();
}

void CRobloxWnd::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	if (m_created) {
		OnSize(event->size().width(), event->size().height());
	}
}

void CRobloxWnd::processMouse(RBX::UIEvent::EventType eventType, QMouseEvent* event, bool handled)
{
	if (!m_created) {
		return;
	}
	if (m_userInput && m_userInput->isMouseAcquired()) {
		event->accept();
		if (event->type() != QEvent::MouseMove) {
			InputEvent::Button button = event->button() == Qt::LeftButton    ? InputEvent::LeftButton
										: event->button() == Qt::RightButton ? InputEvent::RightButton
																			 : InputEvent::OtherButton;
			InputDevice::postMouseButton(button, event->type() != QEvent::MouseButtonRelease);
		}
		return;
	}
	if (handled) {
		event->accept();
	}
	else {
		event->ignore();
	}
	if (m_dataModel) {
		if (m_glWindow) {
			m_glWindow->makeCurrent();
		}
		if (m_userInput && eventType != RBX::UIEvent::NO_EVENT) {
			QPointF position = event->position();
			m_userInput->mouseMessage(eventType, G3D::Vector2(position.x(), position.y()));
		}
		update();
	}
}

void CRobloxWnd::mousePressEvent(QMouseEvent* event)
{
	RBX::UIEvent::EventType eventType = RBX::UIEvent::NO_EVENT;
	if (event->button() == Qt::LeftButton) {
		eventType = RBX::UIEvent::MOUSE_LEFT_BUTTON_DOWN;
	}
	else if (event->button() == Qt::RightButton) {
		eventType = RBX::UIEvent::MOUSE_RIGHT_BUTTON_DOWN;
	}
	if (eventType != RBX::UIEvent::NO_EVENT && !(m_userInput && m_userInput->isMouseAcquired())) {
		setFocus();
	}
	processMouse(eventType, event, true);
}

void CRobloxWnd::mouseReleaseEvent(QMouseEvent* event)
{
	RBX::UIEvent::EventType eventType = RBX::UIEvent::NO_EVENT;
	if (event->button() == Qt::LeftButton) {
		eventType = RBX::UIEvent::MOUSE_LEFT_BUTTON_UP;
	}
	else if (event->button() == Qt::RightButton) {
		eventType = RBX::UIEvent::MOUSE_RIGHT_BUTTON_UP;
	}
	processMouse(eventType, event, false);
}

void CRobloxWnd::mouseDoubleClickEvent(QMouseEvent* event)
{
	processMouse(RBX::UIEvent::NO_EVENT, event, false);
}

void CRobloxWnd::mouseMoveEvent(QMouseEvent* event)
{
	processMouse(RBX::UIEvent::MOUSE_MOVE, event, true);
}

void CRobloxWnd::wheelEvent(QWheelEvent* event)
{
	if (!m_created) {
		event->ignore();
		return;
	}
	event->accept();
	if (m_userInput && m_userInput->isMouseAcquired()) {
		InputDevice::postMouseWheel(event->angleDelta().y() / 120.0f);
		return;
	}
	if (m_dataModel) {
		if (m_glWindow) {
			m_glWindow->makeCurrent();
		}
		if (m_userInput) {
			QPointF position = event->position();
			m_userInput->mouseWheel(event->angleDelta().y() > 0, G3D::Vector2(position.x(), position.y()));
		}
		update();
	}
}

void CRobloxWnd::keyPressEvent(QKeyEvent* event)
{
	if (m_userInput) {
		m_userInput->keyPressed();
	}
	event->ignore();
}

static Display* x11Display()
{
	if (QNativeInterface::QX11Application* application = qApp->nativeInterface<QNativeInterface::QX11Application>()) {
		return application->display();
	}
	throw std::runtime_error("OpenGL requires an X11 display");
}
#endif

void CRobloxWnd::setActive(bool active)
{
	if (!active && m_userInput) {
		m_userInput->onDeactivate();
	}
}

void CRobloxWnd::focusInEvent(QFocusEvent* event)
{
	QWidget::focusInEvent(event);
	OnSetFocus();
}

void CRobloxWnd::focusOutEvent(QFocusEvent* event)
{
	QWidget::focusOutEvent(event);
	OnKillFocus();
}

void CRobloxWnd::OnSetFocus()
{
	if (m_userInput) {
		m_userInput->setFocused(true);
	}
}

void CRobloxWnd::AdjustThrottle(double frameRate)
{
	std::scoped_lock lock(throttleMutex);
	m_throttle = frameRate;
}

void CRobloxWnd::createRenderDevice(G3D::GWindow* window)
{
	m_glWindow = window;
	window->makeCurrent();

	if (G3D::GLCaps::renderer() == "GDI Generic") {
		throw std::runtime_error(
			"This computer does not have correctly installed graphics drivers and is using the default Microsoft "
			"OpenGL implementation. Most graphics capabilities are disabled. To correct this problem, download and "
			"install the latest drivers for the graphics card."
		);
	}

	m_renderDevice.reset(new G3D::RenderDevice());
	if (!m_renderDevice->init(m_glWindow, G3D::Log::common())) {
		m_renderDevice.reset();
		throw std::runtime_error("renderDevice->init failed");
	}

	RBX::Render::SpecData::glLoaded = true;
	m_renderDevice->setSwapBuffersAutomatically(false);
	if (m_alpha) {
		m_renderDevice->enableAlphaWrite();
	}

	if (glGetError() != GL_NO_ERROR) {
		throw std::runtime_error("GL error creating RenderDevice");
	}
}

void CRobloxWnd::destroyRenderDevice()
{
	if (m_glWindow) {
		m_glWindow->makeCurrent();
	}

	if (m_renderDevice.get()) {
		m_renderDevice->cleanup();
		m_renderDevice.reset();
	}

	if (m_glWindow) {
		delete m_glWindow;
		m_glWindow = nullptr;
	}
}

void CRobloxWnd::destroyGraphics()
{
	if (m_graphicsMode == CRenderSettings::c_graphicsModeOpenGLStable) {
		destroyRenderDevice();
	}
	else if (m_visualEngine) {
		m_adorn.reset();
		delete m_visualEngine;
		m_visualEngine = nullptr;
	}
}

void CRobloxWnd::swapBuffers()
{
	if (m_swapPending) {
		ScopedStopwatch timer(m_view->getRenderStats().swapBuffer);
		m_glWindow->swapGLBuffers();
		m_swapPending = false;
	}
}

void CRobloxWnd::OnKillFocus()
{
	swapBuffers();
	if (m_userInput) {
		m_userInput->setFocused(false);
	}
}

CRobloxWnd::CRobloxWnd(bool alpha, QWidget* parent)
	: QWidget(parent), m_workspace(nullptr), m_firstFrame(true), m_alpha(alpha), m_rendering(false),
	  m_swapPending(false), m_swapImmediately(true), m_throttle(666.0),
	  m_profiler(new RBX::Profiling::CodeProfiler("Render")), m_glWindow(nullptr), m_visualEngine(nullptr),
	  m_created(false)
{
	m_fallbackCount = 0;
	m_graphicsMode = CRenderSettings::singleton().getLatchedGraphicsMode();
	if (m_graphicsMode == CRenderSettings::c_graphicsModeAutomatic ||
		m_graphicsMode == CRenderSettings::c_graphicsModeDirect3D) {
		m_graphicsMode = rbxGfxGraphicsMode;
	}

	setWindowFlag(Qt::MSWindowsOwnDC);
	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setFocusPolicy(Qt::StrongFocus);
	setCursor(Qt::ArrowCursor);
#ifndef _WIN32
	setMouseTracking(true);
#endif
	resize(std::max(width(), 100), std::max(height(), 100));
}

CRobloxWnd::~CRobloxWnd()
{
	OnDestroy();
	setWorkspace(nullptr);
}

void CRobloxWnd::InvalidateRunView()
{
	if (m_created) {
#ifdef _WIN32
		InvalidateRgn(reinterpret_cast<HWND>(winId()), nullptr, FALSE);
#else
		QMetaObject::invokeMethod(this, [this]() { update(); }, Qt::QueuedConnection);
#endif
	}
}

void CRobloxWnd::applyThrottle()
{
	double frameRate;
	{
		std::scoped_lock lock(throttleMutex);
		if (m_throttle == 666.0) {
			return;
		}
		frameRate = m_throttle;
		m_throttle = 666.0;
	}

	if (m_view.get() == nullptr || m_dataModel == nullptr) {
		return;
	}

	RBX::DataModel::Lock lock(m_dataModel);

	float shadingQuality = m_view->getShadingQuality();
	float meshDetail = m_view->getMeshDetail();
	if (frameRate < -0.01) {
		shadingQuality *= 0.8f;
		meshDetail *= 0.8f;
	}
	else if (frameRate > 0.002) {
		shadingQuality = shadingQuality * 1.05f + 1.0f;
		meshDetail = meshDetail * 1.05f + 1.0f;
	}

	shadingQuality = std::min(shadingQuality, CRenderSettings::singleton().getMaxShadingQuality());
	shadingQuality = std::max(shadingQuality, CRenderSettings::singleton().getMinShadingQuality());
	meshDetail = std::min(meshDetail, CRenderSettings::singleton().getMaxMeshDetail());
	meshDetail = std::max(meshDetail, CRenderSettings::singleton().getMinMeshDetail());

	if (m_dataModel->getWorkspace() && m_dataModel->getWorkspace()->getCamera()) {
		const G3D::CoordinateFrame& cameraFrame = m_dataModel->getWorkspace()->getCamera()->getCameraCoordinateFrame();
		float distance =
			(cameraFrame.translation - m_dataModel->getWorkspace()->getCamera()->getCameraFocus().translation).length();
		if (m_visualEngine != nullptr &&
			!Ogre::ogreRoot->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_VERTEX_PROGRAM)) {
			RBX::ViewBase::getShadowsEnabled() = false;
		}
		m_view->updateSettings(shadingQuality, meshDetail, RBX::ViewBase::getShadowsEnabled(), distance);
	}
}

void CRobloxWnd::OnTimer(unsigned nIDEvent)
{
	switch (nIDEvent) {
	case 0:
		swapBuffers();
		applyThrottle();
		break;
	case 1:
		if (m_dataModel && m_userInput) {
			m_userInput->onTimer();
		}
		break;
	}
}

void CRobloxWnd::createRbxGfx(int cx, int cy)
{
#ifdef _WIN32
	HWND hwnd = reinterpret_cast<HWND>(winId());
	RECT rect;
	GetClientRect(hwnd, &rect);
	Ogre::String windowHandle = Ogre::StringConverter::toString((unsigned long) hwnd);
	int clientWidth = rect.right - rect.left;
	int clientHeight = rect.bottom - rect.top;
#else
	Display* display = x11Display();
	Ogre::String windowHandle = Ogre::StringConverter::toString(reinterpret_cast<unsigned long>(display)) + ":" +
								Ogre::StringConverter::toString(defaultScreen(display)) + ":" +
								Ogre::StringConverter::toString(static_cast<unsigned long>(winId()));
	int clientWidth = width();
	int clientHeight = height();
#endif

	int videoMemory = Ogre::getVideoMemorySize();
	m_visualEngine = new Ogre::VisualEngine();
	if (RBX::ViewBase::getShadowsEnabled()) {
		m_visualEngine->m_largeVertexBufferOverride = 0;
	}

	try {
		int antialiasing;
		switch (CRenderSettings::singleton().m_antialiasingMode) {
		case CRenderSettings::c_antialiasingModeAutomatic:
			antialiasing = videoMemory < 100000000 ? 2 : 0;
			break;
		case CRenderSettings::c_antialiasingModeOn:
			antialiasing = 0;
			break;
		default:
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Unknown antialiasing mode");
			[[fallthrough]];
		case CRenderSettings::c_antialiasingModeOff:
			antialiasing = 2;
			break;
		}

		m_visualEngine->initialise(
			m_graphicsMode != CRenderSettings::c_graphicsModeDirect3D,
			windowHandle,
			clientWidth,
			clientHeight,
			antialiasing,
			LogManager::getMainLogManger()->MakeLogFileName("gfx")
		);
	}
	catch (std::exception& e) {
		if (m_fallbackCount == 0 && CRenderSettings::singleton().getLatchedGraphicsMode() == 1) {
			m_fallbackCount++;
			RBX::StandardOut::singleton()->print(
				RBX::MESSAGE_WARNING, "Direct3D error: \"%s\". Trying OpenGL stable.", e.what()
			);
			m_graphicsMode = CRenderSettings::c_graphicsModeOpenGLStable;
			createG3D(cx, cy);
			return;
		}
		throw std::runtime_error("Graphics failed to initialize");
	}

	Ogre::frameRateManagerEnabled =
		CRenderSettings::singleton().m_frameRateManagerMode != CRenderSettings::c_frameRateManagerModeOff;

	bool bevels;
	switch (CRenderSettings::singleton().m_bevelMode) {
	case CRenderSettings::c_bevelModeAutomatic:
		bevels = videoMemory >= 100000000 && !m_visualEngine->m_softwareVertexProcessing;
		break;
	case CRenderSettings::c_bevelModeOn:
		bevels = true;
		break;
	default:
		bevels = false;
		break;
	}
	RBX::ViewNew::setBevelsEnabled(bevels);
	RBX::View::setBevelsEnabled(bevels);

	Ogre::Animation::setDefaultInterpolationMode(Ogre::Animation::IM_LINEAR);
	Ogre::Animation::setDefaultRotationInterpolationMode(Ogre::Animation::RIM_LINEAR);
	m_visualEngine->m_sceneManager->setAmbientLight(Ogre::ColourValue(0.5f, 0.5f, 0.5f));
	m_visualEngine->m_camera->setPosition(0.0f, 50.0f, 200.0f);
	m_visualEngine->m_camera->lookAt(0.0f, 50.0f, 0.0f);
}

void CRobloxWnd::createG3D(int width, int height)
{
	G3D::GWindow::Settings settings;
	settings.width = width;
	settings.height = height;
	settings.stencilBits = 8;
	settings.rgbBits = 8;
	settings.depthBits = 24;
	settings.alphaBits = m_alpha ? 8 : 0;
	settings.fsaaSamples = aaSamples;

#ifdef _WIN32
	HWND hwnd = reinterpret_cast<HWND>(winId());
	HDC dc = GetDC(hwnd);
#endif

	G3D::GWindow* window;
	try {
#ifdef _WIN32
		window = G3D::Win32Window::create(settings, dc);
		ReleaseDC(hwnd, dc);
#else
		window = new GlxWindow(settings, x11Display(), winId());
#endif
	}
	catch (std::exception&) {
#ifdef _WIN32
		ReleaseDC(hwnd, dc);
#endif
		if (CRenderSettings::singleton().getLatchedGraphicsMode() == 1) {
			CRenderSettings::singleton().setGraphicsMode(rbxGfxGraphicsMode);
			RBX::GlobalSettings::singleton()->saveState();
			SystemMessageBox::warning(
				nullptr,
				"Roblox",
				"There has been an error in graphics initialization. ROBLOX has made changes to your setting which "
				"will take effect after you restart. Please restart ROBLOX and try again."
			);
			QCoreApplication::quit();
			throw;
		}
		throw std::runtime_error("Graphics failed to initialize");
	}

	try {
		createRenderDevice(window);
	}
	catch (std::exception& e) {
		if (m_fallbackCount == 0 && CRenderSettings::singleton().getLatchedGraphicsMode() == 1) {
			m_fallbackCount++;
			RBX::StandardOut::singleton()->print(
				RBX::MESSAGE_WARNING, "OpenGL-G3D failed: \"%s\". Trying RbxGfx.", e.what()
			);
			m_graphicsMode = rbxGfxGraphicsMode;
			createRbxGfx(width, height);
		}
		else {
			throw std::runtime_error("Graphics failed to initialize");
		}
	}
}

bool CRobloxWnd::create()
{
#ifdef _WIN32
	HWND hwnd = reinterpret_cast<HWND>(winId());
	SetClassLongPtrA(hwnd, GCL_STYLE, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC);
	RECT rect;
	GetClientRect(hwnd, &rect);
	int cy = rect.bottom - rect.top;
	int cx = rect.right - rect.left;
#else
	winId();
	int cy = height();
	int cx = width();
#endif

	try {
		if (m_graphicsMode == CRenderSettings::c_graphicsModeOpenGLStable) {
			createG3D(cx, cy);
		}
		else {
			createRbxGfx(cx, cy);
		}
	}
	catch (std::exception& e) {
		SystemMessageBox::warning(nullptr, "Roblox", QStringLiteral("Unable to run Roblox.\n\n%1").arg(e.what()));
		return false;
	}

#ifdef _WIN32
	SetTimer(hwnd, 0, 200, nullptr);
	SetTimer(hwnd, 1, 100, nullptr);
#else
	m_frameTimer = new QTimer(this);
	connect(m_frameTimer, &QTimer::timeout, this, [this]() { OnTimer(0); });
	m_frameTimer->start(200);
	m_inputTimer = new QTimer(this);
	connect(m_inputTimer, &QTimer::timeout, this, [this]() { OnTimer(1); });
	m_inputTimer->start(100);
#endif
	idleWindows.insert(this);
	m_created = true;
#ifndef _WIN32
	OnSize(width(), height());
#endif
	return true;
}

void CRobloxWnd::resizeG3D(int cx, int cy)
{
	if (m_glWindow && m_renderDevice.get()) {
		std::string renderer = G3D::GLCaps::renderer();
		if (G3D::beginsWith(renderer, "MOBILITY RADEON 7500")) {
			destroyGraphics();
			try {
				if (m_graphicsMode == CRenderSettings::c_graphicsModeOpenGLStable) {
					createG3D(cx, cy);
				}
				else {
					createRbxGfx(cx, cy);
				}
				if (m_glWindow) {
					m_glWindow->makeCurrent();
				}
			}
			catch (std::exception&) {
			}
		}
		else {
			m_renderDevice->notifyResize(cx, cy);
			G3D::Rect2D viewport = G3D::Rect2D::xywh(0, 0, m_renderDevice->width(), m_renderDevice->height());
			m_renderDevice->setViewport(viewport);
		}
		m_swapImmediately = true;
	}
}

void CRobloxWnd::OnSize(int cx, int cy)
{
	if (cx > 0 && cy > 0 && cx <= 0x2000 && cy <= 0x2000) {
		if (m_graphicsMode == CRenderSettings::c_graphicsModeOpenGLStable) {
			resizeG3D(cx, cy);
		}
		else {
			resizeRbxGfx(cx, cy);
		}
	}
}

void CRobloxWnd::createG3DView()
{
	m_view.reset(new RBX::View::View(m_dataModel));
	m_adorn.reset(new RBX::AdornG3D(m_renderDevice.get()));
}

void CRobloxWnd::renderG3D(bool draw2d)
{
	m_renderDevice->pushState();
	try {
		RBX::DataModel::Lock lock(m_dataModel);
		RenderFrame frame(m_dataModel.get());
		m_view->render(m_renderDevice.get());
		ScopedStopwatch timer(m_view->getRenderStats().render2D);
		m_dataModel->renderPass3dAdorn(m_adorn.get());
		if (draw2d) {
			m_renderDevice->push2D();
			m_dataModel->renderPass2d(m_adorn.get(), this);
			m_renderDevice->pop2D();
		}
	}
	catch (std::exception& e) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
	}
	runViewValid(m_dataModel.get(), this);
	m_renderDevice->popState();
}

void CRobloxWnd::renderRbxGfx(bool draw2d)
{
	if (RBX::Network::Players::findLocalCharacter(m_dataModel.get()) == nullptr) {
		Ogre::frameRateManagerEnabled = false;
	}
	else {
		Ogre::frameRateManagerEnabled =
			CRenderSettings::singleton().m_frameRateManagerMode != CRenderSettings::c_frameRateManagerModeOff;
	}

	Ogre::Timer timer;
	float milliseconds;
	{
		ScopedStopwatch cpuTimer(m_view->getRenderStats().cpuRenderTotal);
		timer.reset();
		RBX::DataModel::Lock lock(m_dataModel);
		RenderFrame frame(m_dataModel.get());
		m_dataModel->renderPass2d(m_adorn.get(), this);
		m_dataModel->renderPass3dAdorn(m_adorn.get());
		m_adorn->prepareRenderPass();
		m_view->render(m_visualEngine);
		m_adorn->finishRenderPass();
		milliseconds = timer.getMilliseconds();
	}

	if (Ogre::frameRateManagerEnabled) {
		m_visualEngine->getFrameRateManager().getFrame(0).m_frameTime = milliseconds;
	}

	ScopedStopwatch render2DTimer(m_view->getRenderStats().render2D);
	runViewValid(m_dataModel.get(), this);
}

void CRobloxWnd::render(bool draw2d)
{
	if (!m_rendering) {
		RBX::ScopedAssign<bool> rendering(m_rendering, true);
		if (!m_dataModel) {
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		else if (!m_view.get()) {
			runViewValid(m_dataModel.get(), this);
		}
		else if (m_graphicsMode == CRenderSettings::c_graphicsModeOpenGLStable) {
			renderG3D(draw2d);
		}
		else {
			renderRbxGfx(draw2d);
		}
	}
}

bool CRobloxWnd::paintRbxGfx()
{
#ifdef _WIN32
	HWND hwnd = reinterpret_cast<HWND>(winId());
	if (!repaintRequested) {
		bool resized = false;
		{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			RECT paint = ps.rcPaint;
			if (paint.right != 0 || paint.left != 0 || paint.top != 0 || paint.bottom != 0) {
				if (paint.right - paint.left + 1 < (int) m_visualEngine->m_renderWindow->getWidth() ||
					paint.bottom - paint.top + 1 < (int) m_visualEngine->m_renderWindow->getHeight()) {
					resized = true;
				}
			}
			EndPaint(hwnd, &ps);
		}
		if (resized) {
			InvalidateRgn(GetAncestor(hwnd, GA_ROOT), nullptr, FALSE);
			repaintRequested = true;
			return false;
		}
	}
	else {
		repaintRequested = false;
	}
#endif
	render(true);
	return true;
}

bool CRobloxWnd::paintG3D()
{
	bool painted = false;
#ifdef _WIN32
	HWND hwnd = reinterpret_cast<HWND>(winId());
	PAINTSTRUCT ps;
	HDC dc = BeginPaint(hwnd, &ps);
	if (m_glWindow && dc && m_dataModel && !IsRectEmpty(&ps.rcPaint)) {
#else
	if (m_glWindow && m_dataModel) {
#endif
		m_glWindow->makeCurrent();
		if (!m_renderDevice.get()) {
#ifdef _WIN32
			RECT rect;
			GetClientRect(hwnd, &rect);
			DrawTextA(dc, "Roblox can't run with your graphics driver", -1, &rect, DT_END_ELLIPSIS);
#endif
		}
		else {
			RBX::Profiling::Mark mark(*m_profiler, true);
			swapBuffers();
			m_renderDevice->beginFrame();
			render(true);
			painted = true;
			m_renderDevice->endFrame();
			m_swapPending = true;
			if (m_swapImmediately || !prop_delaySwapBuffer.getValue(&CRenderSettings::singleton()) ||
				!m_dataModel->canDelaySwapBuffer()) {
				swapBuffers();
				m_swapImmediately = false;
			}
			if (m_firstFrame) {
				m_firstFrame = false;
				RBX::Render::Mesh::makeVAR();
				RBX::Render::Mesh::varDirty = true;
			}
		}
	}
#ifdef _WIN32
	EndPaint(hwnd, &ps);
#endif
	return painted;
}

void CRobloxWnd::OnPaint()
{
	if (!m_created) {
#ifdef _WIN32
		HWND hwnd = reinterpret_cast<HWND>(winId());
		PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
#endif
		return;
	}

	bool painted;
	if (m_graphicsMode == CRenderSettings::c_graphicsModeOpenGLStable) {
		painted = paintG3D();
	}
	else {
		painted = paintRbxGfx();
	}
	if (!painted && m_dataModel) {
		runViewValid(m_dataModel.get(), this);
	}
}

bool CRobloxWnd::isIdleActive() const
{
	return idleWindows.contains(const_cast<CRobloxWnd*>(this)) && m_dataModel && isVisible() &&
		   !window()->isMinimized();
}

bool CRobloxWnd::hasActiveWindow()
{
	return std::any_of(idleWindows.begin(), idleWindows.end(), [](CRobloxWnd* window) {
		return window->isIdleActive();
	});
}

bool CRobloxWnd::renderIdle(double& wait)
{
	std::vector<CRobloxWnd*> windows(idleWindows.begin(), idleWindows.end());
	bool active = false;
	bool rendered = false;
	for (CRobloxWnd* window : windows) {
		if (!idleWindows.contains(window) || !window->isIdleActive()) {
			continue;
		}

		active = true;
		if (RBX::getRealTime() < RBX::FrameInterpolator::getNextStepTime()) {
			RBX::FrameInterpolator::setExtraFrame(true);
#ifdef _WIN32
			RedrawWindow(reinterpret_cast<HWND>(window->winId()), nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
#else
			window->repaint();
#endif
			RBX::FrameInterpolator::setExtraFrame(false);
			rendered = true;
		}
	}

	wait = 0.0;
	if (active && !rendered) {
		double remaining = RBX::FrameInterpolator::getNextStepTime() - RBX::getRealTime();
		wait = remaining > 0.0 ? remaining : 0.0005;
	}

	return active;
}

std::string CRobloxWnd::getMetric(const std::string& metric) const
{
	if (metric == "graphicsMode") {
		return RBX::Reflection::EnumDesc<CRenderSettings::GraphicsMode>::singleton().convertToString(m_graphicsMode);
	}

	if (metric == "FRM") {
		if (!m_visualEngine) {
			return "N/A";
		}
		if (!Ogre::frameRateManagerEnabled && m_visualEngine->m_frameRateManager.getFrameCount() < 30) {
			return "Off";
		}

		Ogre::VisualEngine::FrameRateManager::FrameStats& frame = m_visualEngine->getFrameRateManager().getFrame(0);
		int visible = (int) frame.m_visibleCount;
		int rendered = (int) frame.m_renderedCount;
		int value = 0;
		if (visible > 0) {
			value = (int) ((1.0 - (double) rendered / visible) * 100.0);
		}

		int subEntities = 0;
		Ogre::SceneNode* root = m_visualEngine->m_sceneManager->getRootSceneNode();
		for (int i = 0; i < root->numChildren(); i++) {
			Ogre::RbxSceneNode* node = dynamic_cast<Ogre::RbxSceneNode*>(root->getChild(i));
			if (node) {
				if (Ogre::RbxEntity* entity = dynamic_cast<Ogre::RbxEntity*>(node->getAttachedObject(0))) {
					subEntities += entity->getNumSubEntities();
				}
			}
		}

		char buffer[128];
		std::snprintf(buffer, sizeof(buffer), "On  %2d%% r=%d v=%d w=%d", value, rendered, visible, subEntities);
		return buffer;
	}

	if (metric == "Shadows") {
		if (!m_visualEngine) {
			return "?";
		}
		if (!RBX::ViewBase::getShadowsEnabled()) {
			return "Off";
		}
		Ogre::RbxSceneManager* sceneManager = static_cast<Ogre::RbxSceneManager*>(m_visualEngine->m_sceneManager);
		int casters = (int) sceneManager->getShadowCasterList().size();
		return "On  #casters=" + Ogre::StringConverter::toString(casters);
	}

	if (metric == "antiAliasing") {
		if (!m_visualEngine) {
			return "N/A";
		}
		std::string result;
		switch (m_visualEngine->m_antialiasing) {
		case 1:
			result = "On";
			break;
		case 2:
			result = "Off";
			break;
		}
		return result;
	}

	if (metric == "bevels") {
		if (!m_visualEngine) {
			return "N/A";
		}
		return RBX::ViewNew::getBevelsEnabled() ? "On" : "Off";
	}

	if (metric == "videoMemory") {
		return RBX::StringConverter<int>::convertToString(Ogre::getVideoMemorySize());
	}

	if (metric == "GfxTime") {
		if (!m_visualEngine) {
			return "?";
		}
		float ms;
		if (Ogre::frameRateManagerEnabled) {
			if (m_visualEngine->m_frameRateManager.getFrameCount() == 0) {
				return "??";
			}
			ms = m_visualEngine->getFrameRateManager().getFrame(0).m_frameTime;
		}
		else {
			ms = m_view->getRenderStats().cpuRenderTotal.elapsedTime();
			ms *= 1000.0;
		}
		double fps = ms >= 1.0 ? 1000.0 / ms : 1000.0;

		char buffer[256];
		std::snprintf(buffer, sizeof(buffer), "%.1fms (%.1ffps)", ms, fps);
		return buffer;
	}

	return "";
}

CRobloxWnd::RenderStatsItem::RenderStatsItem(RenderStats* stats)
{
	m_stats = stats;
	createBoundChildItem("2D CPU Render", stats->render2D);

	RBX::Stats::Item* total = createBoundChildItem("3D CPU Total", stats->cpuRenderTotal);
	total->createBoundChildItem("CPU Shadow", stats->cpuShadow);
	total->createBoundChildItem("computeProxyArrays", stats->computeProxyArrays);
	total->createBoundChildItem("Sort", stats->sort);
	total->createBoundChildItem("Proxy count", stats->diffuseProxyCount);

	RBX::Stats::Item* triangles = total->createBoundChildItem("Total Triangles", stats->totalTriangles);
	triangles->createBoundChildItem("Ambient + Unshadowed", stats->unshadowedTriangles);
	triangles->createBoundChildItem("Mark Shadows", stats->markShadowsTriangles);
	triangles->createBoundChildItem("Shadowed Light", stats->shadowedLightTriangles);

	createBoundChildItem("SwapBuffer", stats->swapBuffer);

	m_aggregation = createChildItem("Aggregation");
	m_aggregation->createBoundChildItem("Total Chunks", RenderStats::chunkCount);
	m_aggregation->createBoundChildItem("Aggregated Chunks", RenderStats::aggregatedChunkCount);
	m_aggregation->createBoundChildItem("AggregateChunks", RenderStats::aggregateChunkCount);

	RBX::Stats::Item* memory = createChildItem("Memory");
	m_textureMemory = memory->createChildItem("Tex");
	m_varMemory = memory->createChildItem("VAR");
	m_videoMemory = memory->createChildItem("Video");

	RBX::Stats::Item* stateChanges = createChildItem("State Changes");
	m_majorStateChanges = stateChanges->createChildItem("Major");
	m_minorStateChanges = stateChanges->createChildItem("Minor");
}

void CRobloxWnd::RenderStatsItem::update()
{
	int chunkCount = RenderStats::chunkCount;
	if (chunkCount == 0) {
		m_aggregation->setValue(0.0, "n/a");
	}
	else if (RenderStats::aggregateChunkCount == 0) {
		m_aggregation->formatValue(0.0, "%.0f%%, %.2g", 0.0, 1.0);
	}
	else {
		float total = chunkCount;
		float rendered = RenderStats::aggregateChunkCount - RenderStats::aggregatedChunkCount + chunkCount;
		float ratio = (total - rendered) / (total - 1.0);
		m_aggregation->formatValue(ratio, "%.0f%%, %.2g", ratio * 100.0, total / rendered);
	}

	m_majorStateChanges->formatValue(
		m_stats->majorGLStateChanges, "%d/%d", m_stats->majorGLStateChanges, m_stats->majorStateChanges
	);
	m_minorStateChanges->formatValue(
		m_stats->minorGLStateChanges, "%d/%d", m_stats->minorGLStateChanges, m_stats->minorStateChanges
	);

	m_textureMemory->formatMem(G3D::Texture::sizeOfAllTexturesInMemory());
	m_varMemory->formatMem(G3D::VARArea::sizeOfAllVARAreasInMemory());
	m_videoMemory->formatMem(G3D::DXCaps::videoMemorySize());
}

void CRobloxWnd::setWorkspace(CWorkspace* workspace)
{
	if (workspace != m_workspace) {
		if (m_dataModel) {
			RBX::DataModel::Lock lock(m_dataModel);
			if (m_renderStatsItem) {
				m_renderStatsItem->setParent(nullptr);
				m_renderStatsItem.reset();
			}
			if (RBX::ControllerService* controllers = m_dataModel->find<RBX::ControllerService>()) {
				controllers->setHardwareDevice(nullptr);
			}
			if (m_glWindow) {
				m_glWindow->makeCurrent();
			}
			m_view.reset();
			removeRunView(m_dataModel.get(), this);
			m_userInput.reset();
		}

		m_workspace = workspace;
		if (workspace) {
			m_dataModel = workspace->dataModel();
		}
		else {
			m_dataModel.reset();
		}

		if (m_dataModel) {
			m_userInput.reset(new UserInput(winId(), m_dataModel.get()));

			RBX::DataModel::Lock lock(m_dataModel);
			CRenderSettings::singleton().updateQualityProfile(false);
			if (m_graphicsMode == CRenderSettings::c_graphicsModeOpenGLStable) {
				createG3DView();
			}
			else {
				createRbxGfxView();
			}
			m_userInput->init(m_adorn.get());
			AdjustThrottle(0.0);
			applyThrottle();

			m_dataModel->create<RBX::ControllerService>()->setHardwareDevice(m_userInput.get());
			RBX::Stats::StatsService* stats = m_dataModel->create<RBX::Stats::StatsService>();

			m_renderStatsItem = RBX::Creatable<RBX::Instance>::create<RenderStatsItem>(&m_view->getRenderStats());
			m_renderStatsItem->setName("Render");
			m_renderStatsItem->setParent(stats);
			m_renderStatsItem->createBoundChildItem(*m_profiler);

			m_dataModel->create<RBX::RunService>()->addRunView(this);
		}
	}
	if (m_created) {
#ifdef _WIN32
		InvalidateRect(reinterpret_cast<HWND>(winId()), nullptr, TRUE);
#else
		update();
#endif
	}
}

void CRobloxWnd::OnDestroy()
{
	if (!m_created) {
		return;
	}
	m_created = false;

	idleWindows.erase(this);
	swapBuffers();
	setWorkspace(nullptr);
	destroyGraphics();
}
