#include "RenderSettings.h"

#include "View.h"
#include "ViewBase.h"
#include "rbxview/View.h"
#include "renderlib/AggregatingSceneManager.h"
#include "renderlibnew/Chunk.h"

#include <GLG3D/DXCaps.h>

const float defaultMinShadingQuality = 20.0f;
const float defaultMaxShadingQuality = 20.0f;
const float defaultMinMeshDetail = 50.0f;
const float defaultMaxMeshDetail = 50.0f;

const char sRenderSettings[] = "RenderSettings";

CRenderSettings::AASamples aaSamples = CRenderSettings::c_aaSamplesNone;
G3D::Vector2int16 defaultWindowSize(800, 600);
CRenderSettings::GraphicsMode latchedGraphicsMode;

static RBX::Reflection::EnumPropDescriptor<CRenderSettings, CRenderSettings::GraphicsMode> prop_graphicsMode(
	"graphicsMode", "General", &CRenderSettings::getGraphicsMode, &CRenderSettings::setGraphicsMode
);

static RBX::Reflection::EnumPropDescriptor<CRenderSettings, CRenderSettings::FrameRateManagerMode>
	prop_frameRateManager(
		"frameRateManager",
		"General",
		&CRenderSettings::getFrameRateManagerMode,
		&CRenderSettings::setFrameRateManagerMode
	);

static RBX::Reflection::EnumPropDescriptor<CRenderSettings, CRenderSettings::BevelMode> prop_Bevels(
	"Bevels", "General", &CRenderSettings::getBevelMode, &CRenderSettings::setBevelMode
);

static RBX::Reflection::EnumPropDescriptor<CRenderSettings, CRenderSettings::BevelShadingMode> prop_BevelShading(
	"BevelShading", "General", &CRenderSettings::getBevelShadingMode, &CRenderSettings::setBevelShadingMode
);

static RBX::Reflection::PropDescriptor<CRenderSettings, float> prop_BevelLODDistance(
	"BevelLODDistance", "General", &CRenderSettings::getBevelLodDistance, &CRenderSettings::setBevelLodDistance
);

static RBX::Reflection::EnumPropDescriptor<CRenderSettings, CRenderSettings::ShadowMode> prop_Shadow(
	"Shadow", "General", &CRenderSettings::getShadowMode, &CRenderSettings::setShadowMode
);

static RBX::Reflection::EnumPropDescriptor<CRenderSettings, CRenderSettings::AntialiasingMode> prop_Antialiasing(
	"Antialiasing", "General", &CRenderSettings::getAntialiasingMode, &CRenderSettings::setAntialiasingMode
);

static RBX::Reflection::PropDescriptor<CRenderSettings, G3D::Vector2int16> prop_WindowSize(
	"WindowSize", "Screen", &CRenderSettings::getWindowSize, &CRenderSettings::setWindowSize
);

static RBX::Reflection::PropDescriptor<CRenderSettings, G3D::Vector2int16> prop_FullscreenSize(
	"FullscreenSize", "Screen", &CRenderSettings::getFullscreenSize, &CRenderSettings::setFullscreenSize
);

static RBX::Reflection::PropDescriptor<CRenderSettings, float> prop_minShadingQuality(
	"minShadingQuality", "Qaulity", &CRenderSettings::getMinShadingQuality, &CRenderSettings::setMinShadingQuality
);

static RBX::Reflection::PropDescriptor<CRenderSettings, float> prop_maxShadingQuality(
	"maxShadingQuality", "Qaulity", &CRenderSettings::getMaxShadingQuality, &CRenderSettings::setMaxShadingQuality
);

static RBX::Reflection::PropDescriptor<CRenderSettings, float> prop_minMeshDetail(
	"minMeshDetail", "Qaulity", &CRenderSettings::getMinMeshDetail, &CRenderSettings::setMinMeshDetail
);

static RBX::Reflection::PropDescriptor<CRenderSettings, float> prop_maxMeshDetail(
	"maxMeshDetail", "Qaulity", &CRenderSettings::getMaxMeshDetail, &CRenderSettings::setMaxMeshDetail
);

static RBX::Reflection::PropDescriptor<CRenderSettings, float> prop_AggregateSize(
	"_AggregateSize", "Performance", &CRenderSettings::getAggregateSize, &CRenderSettings::setAggregateSize
);

static RBX::Reflection::EnumPropDescriptor<CRenderSettings, CRenderSettings::AASamples> prop_AASamples(
	"AASamples", "Qaulity", &CRenderSettings::getAASamples, &CRenderSettings::setAASamples
);

RBX::Reflection::BoundProp<bool, 1> prop_delaySwapBuffer(
	"delaySwapBuffer", "Performance", &CRenderSettings::m_delaySwapBuffer
);

static RBX::Reflection::BoundProp<std::string, 1> prop_profileName(
	"profileName", "Qaulity", &CRenderSettings::m_profileName, RBX::Reflection::PropertyDescriptor::STREAMING
);

static const char* currentQualityProfile = "profiled5";

static G3D::Vector2int16 defaultFullscreenSize()
{
	if (static_cast<unsigned int>(G3D::DXCaps::videoMemorySize()) >= 16000000) {
		return G3D::Vector2int16(1024, 768);
	}
	return G3D::Vector2int16(800, 600);
}

CRenderSettings::BevelMode CRenderSettings::getBevelMode() const
{
	return m_bevelMode;
}

CRenderSettings::BevelShadingMode CRenderSettings::getBevelShadingMode() const
{
	return m_bevelShadingMode;
}

float CRenderSettings::getBevelLodDistance() const
{
	return m_bevelLodDistance;
}

CRenderSettings::AntialiasingMode CRenderSettings::getAntialiasingMode() const
{
	return m_antialiasingMode;
}

CRenderSettings::AASamples CRenderSettings::getAASamples() const
{
	return aaSamples;
}

G3D::Vector2int16 CRenderSettings::getFullscreenSize() const
{
	return m_fullscreenSize;
}

G3D::Vector2int16 CRenderSettings::getWindowSize() const
{
	return m_windowSize;
}

int CRenderSettings::getAggregateSize() const
{
	return RBX::Render::AggregatingSceneManager::aggregationSize;
}

template <>
RBX::Reflection::EnumDesc<CRenderSettings::AASamples>::EnumDesc()
	: EnumDescriptor("AASamples", typeid(CRenderSettings::AASamples))
{
	addPair(CRenderSettings::c_aaSamplesNone, "None");
	addPair(CRenderSettings::c_aaSamples4, "4");
	addPair(CRenderSettings::c_aaSamples8, "8");
}

template <>
RBX::Reflection::EnumDesc<CRenderSettings::GraphicsMode>::EnumDesc()
	: EnumDescriptor("GraphicsMode", typeid(CRenderSettings::GraphicsMode))
{
	addPair(CRenderSettings::c_graphicsModeAutomatic, "Automatic");
	addPair(CRenderSettings::c_graphicsModeOpenGLStable, "OpenGL stable");
#ifdef _WIN32
	addPair(CRenderSettings::c_graphicsModeDirect3D, "Direct3D experimental");
#endif
	addPair(CRenderSettings::c_graphicsModeOpenGL, "OpenGL experimental");
	addPair(CRenderSettings::c_graphicsModeNoGraphics, "No Graphics");
}

template <>
RBX::Reflection::EnumDesc<CRenderSettings::FrameRateManagerMode>::EnumDesc()
	: EnumDescriptor("Framerate Manager", typeid(CRenderSettings::FrameRateManagerMode))
{
	addPair(CRenderSettings::c_frameRateManagerModeAutomatic, "Automatic");
	addPair(CRenderSettings::c_frameRateManagerModeOn, "On");
	addPair(CRenderSettings::c_frameRateManagerModeOff, "Off");
}

template <>
RBX::Reflection::EnumDesc<CRenderSettings::AntialiasingMode>::EnumDesc()
	: EnumDescriptor("Antialiasing", typeid(CRenderSettings::AntialiasingMode))
{
	addPair(CRenderSettings::c_antialiasingModeAutomatic, "Automatic");
	addPair(CRenderSettings::c_antialiasingModeOff, "Off");
	addPair(CRenderSettings::c_antialiasingModeOn, "On");
}

template <>
RBX::Reflection::EnumDesc<CRenderSettings::BevelMode>::EnumDesc()
	: EnumDescriptor("Bevels", typeid(CRenderSettings::BevelMode))
{
	addPair(CRenderSettings::c_bevelModeAutomatic, "Automatic");
	addPair(CRenderSettings::c_bevelModeOn, "On");
	addPair(CRenderSettings::c_bevelModeOff, "Off");
}

template <>
RBX::Reflection::EnumDesc<CRenderSettings::BevelShadingMode>::EnumDesc()
	: EnumDescriptor("BevelShading", typeid(CRenderSettings::BevelShadingMode))
{
	addPair(CRenderSettings::c_bevelShadingFlat, "Flat");
	addPair(CRenderSettings::c_bevelShadingSmooth, "Smooth");
}

template <>
RBX::Reflection::EnumDesc<CRenderSettings::ShadowMode>::EnumDesc()
	: EnumDescriptor("Shadow", typeid(CRenderSettings::ShadowMode))
{
	addPair(CRenderSettings::c_shadowModeAutomatic, "Automatic");
	addPair(CRenderSettings::c_shadowModeOn, "On");
	addPair(CRenderSettings::c_shadowModeOff, "Off");
}

CRenderSettings::CRenderSettings()
	: m_graphicsMode(c_graphicsModeAutomatic), m_bevelMode(c_bevelModeAutomatic),
	  m_bevelShadingMode(c_bevelShadingFlat), m_bevelLodDistance(80.0f), m_antialiasingMode(c_antialiasingModeOff),
	  m_shadowMode(c_shadowModeAutomatic), m_frameRateManagerMode(c_frameRateManagerModeAutomatic),
	  m_minShadingQuality(defaultMinShadingQuality), m_maxShadingQuality(defaultMaxShadingQuality),
	  m_minMeshDetail(defaultMinMeshDetail), m_maxMeshDetail(defaultMaxMeshDetail), m_delaySwapBuffer(true),
	  m_fullscreenSize(defaultFullscreenSize()), m_windowSize(defaultWindowSize)
{
	setName("Rendering");
	RBX::Render::AggregatingSceneManager::aggregationSize = 1;
}

void CRenderSettings::setAASamples(AASamples value)
{
	if (value != aaSamples) {
		aaSamples = value;
		raisePropertyChanged(prop_AASamples);
	}
}

void CRenderSettings::setFullscreenSize(G3D::Vector2int16 value)
{
	if (value != m_fullscreenSize) {
		m_fullscreenSize = value;
		raisePropertyChanged(prop_FullscreenSize);
	}
}

void CRenderSettings::setWindowSize(G3D::Vector2int16 value)
{
	if (value != m_windowSize) {
		m_windowSize = value;
		raisePropertyChanged(prop_WindowSize);
	}
}

void CRenderSettings::setMinShadingQuality(float value)
{
	if (value > m_maxShadingQuality) {
		setMaxShadingQuality(value);
	}

	value = std::min(m_maxShadingQuality, value);
	value = std::max(0.0f, value);
	if (value != m_minShadingQuality) {
		m_minShadingQuality = value;
		raisePropertyChanged(prop_minShadingQuality);
	}
}

void CRenderSettings::setMaxShadingQuality(float value)
{
	if (value < m_minShadingQuality) {
		setMinShadingQuality(value);
	}

	value = std::max(m_minShadingQuality, value);
	value = std::min(100.0f, value);
	if (value != m_maxShadingQuality) {
		m_maxShadingQuality = value;
		raisePropertyChanged(prop_maxShadingQuality);
	}
}

void CRenderSettings::setMinMeshDetail(float value)
{
	value = std::min(m_maxMeshDetail, std::max(0.0f, value));
	if (value != m_minMeshDetail) {
		m_minMeshDetail = value;
		raisePropertyChanged(prop_minMeshDetail);
	}
}

void CRenderSettings::setMaxMeshDetail(float value)
{
	value = std::min(100.0f, std::max(m_minMeshDetail, value));
	if (value != m_maxMeshDetail) {
		m_maxMeshDetail = value;
		raisePropertyChanged(prop_maxMeshDetail);
	}
}

void CRenderSettings::setAggregateSize(int value)
{
	int size = std::min(1000, std::max(1, value));
	if (size != RBX::Render::AggregatingSceneManager::aggregationSize) {
		RBX::Render::AggregatingSceneManager::aggregationSize = size;
		raisePropertyChanged(prop_AggregateSize);
	}
}

void CRenderSettings::setGraphicsMode(GraphicsMode value)
{
	if (value != m_graphicsMode) {
		m_graphicsMode = value;
		raisePropertyChanged(prop_graphicsMode);
	}
}

void CRenderSettings::setFrameRateManagerMode(FrameRateManagerMode value)
{
	if (value != m_frameRateManagerMode) {
		m_frameRateManagerMode = value;
		raisePropertyChanged(prop_frameRateManager);
	}
}

void CRenderSettings::setBevelMode(BevelMode value)
{
	if (value != m_bevelMode) {
		m_bevelMode = value;
		const bool enabled = value != c_bevelModeOff;
		RBX::ViewNew::setBevelsEnabled(enabled);
		RBX::View::setBevelsEnabled(enabled);
		raisePropertyChanged(prop_Bevels);
	}
}

void CRenderSettings::setBevelShadingMode(BevelShadingMode value)
{
	if (value != m_bevelShadingMode) {
		m_bevelShadingMode = value;
		const bool smooth = value == c_bevelShadingSmooth;
		RBX::ViewNew::setBevelSmoothShading(smooth);
		RBX::View::setBevelSmoothShading(smooth);
		raisePropertyChanged(prop_BevelShading);
	}
}

void CRenderSettings::setBevelLodDistance(float value)
{
	value = std::max(0.0f, value);
	if (value != m_bevelLodDistance) {
		m_bevelLodDistance = value;
		RBX::ViewNew::setBevelLodDistance(value);
		RBX::View::setBevelLodDistance(value);
		raisePropertyChanged(prop_BevelLODDistance);
	}
}

void CRenderSettings::setAntialiasingMode(AntialiasingMode value)
{
	if (value != m_antialiasingMode) {
		m_antialiasingMode = value;
		raisePropertyChanged(prop_Antialiasing);
	}
}

void CRenderSettings::setShadowMode(ShadowMode value)
{
	if (value != m_shadowMode) {
		m_shadowMode = value;
		bool enabled = value != c_shadowModeAutomatic && value != c_shadowModeOff;
		if (enabled != RBX::ViewBase::getShadowsEnabled()) {
			RBX::ViewBase::getShadowsEnabled() = enabled;
			raisePropertyChanged(prop_Shadow);
		}
	}
}

void CRenderSettings::updateQualityProfile(bool value)
{
	if (value || m_profileName != currentQualityProfile) {
		setMaxMeshDetail(100.0f);
		setMinMeshDetail(100.0f);
		setMaxMeshDetail(100.0f);
		setMaxShadingQuality(100.0f);
		setMinShadingQuality(100.0f);
		setMaxShadingQuality(100.0f);
		prop_profileName.setValue(this, currentQualityProfile);
	}
}

template class RBX::FactoryProduct<CRenderSettings, RBX::GlobalSettings::Item, sRenderSettings>;
template class RBX::Reflection::Described<
	CRenderSettings,
	sRenderSettings,
	RBX::FactoryProduct<CRenderSettings, RBX::GlobalSettings::Item, sRenderSettings>>;
template class RBX::DescribedCreatable<CRenderSettings, RBX::GlobalSettings::Item, sRenderSettings>;
