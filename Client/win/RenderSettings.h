#ifndef RENDERSETTINGS_H
#define RENDERSETTINGS_H

#include "v8datamodel/GlobalSettings.h"

#include <G3D/Vector2int16.h>
#include <string>

extern const char sRenderSettings[];

extern const float defaultMinShadingQuality;
extern const float defaultMaxShadingQuality;
extern const float defaultMinMeshDetail;
extern const float defaultMaxMeshDetail;
extern G3D::Vector2int16 defaultWindowSize;

class CRenderSettings;

class CRenderSettings : public RBX::GlobalSettingsItem<CRenderSettings, sRenderSettings>
{
public:
	enum GraphicsMode
	{
		c_graphicsModeAutomatic = 1,
		c_graphicsModeOpenGLStable,
		c_graphicsModeDirect3D,
		c_graphicsModeOpenGL,
		c_graphicsModeNoGraphics,
	};

	enum FrameRateManagerMode
	{
		c_frameRateManagerModeAutomatic,
		c_frameRateManagerModeOn,
		c_frameRateManagerModeOff,
	};

	enum BevelMode
	{
		c_bevelModeAutomatic,
		c_bevelModeOn,
		c_bevelModeOff,
	};

	enum BevelShadingMode
	{
		c_bevelShadingSmooth,
		c_bevelShadingFlat,
	};

	enum ShadowMode
	{
		c_shadowModeAutomatic,
		c_shadowModeOn,
		c_shadowModeOff,
	};

	enum AntialiasingMode
	{
		c_antialiasingModeAutomatic,
		c_antialiasingModeOn,
		c_antialiasingModeOff,
	};

	enum AASamples
	{
		c_aaSamplesNone = 1,
		c_aaSamples4 = 4,
		c_aaSamples8 = 8,
	};

	CRenderSettings();

	float getMinShadingQuality() const { return m_minShadingQuality; }

	float getMaxShadingQuality() const { return m_maxShadingQuality; }

	float getMinMeshDetail() const { return m_minMeshDetail; }

	float getMaxMeshDetail() const { return m_maxMeshDetail; }

	BevelMode getBevelMode() const;
	BevelShadingMode getBevelShadingMode() const;
	float getBevelLodDistance() const;
	AntialiasingMode getAntialiasingMode() const;
	AASamples getAASamples() const;
	G3D::Vector2int16 getFullscreenSize() const;
	G3D::Vector2int16 getWindowSize() const;
	int getAggregateSize() const;
	void setAASamples(AASamples value);
	void setFullscreenSize(G3D::Vector2int16 value);
	void setWindowSize(G3D::Vector2int16 value);
	void setMinShadingQuality(float value);
	void setMaxShadingQuality(float value);
	void setMinMeshDetail(float value);
	void setMaxMeshDetail(float value);
	void setAggregateSize(int value);
	void setGraphicsMode(GraphicsMode value);
	void updateQualityProfile(bool value);
	void setFrameRateManagerMode(FrameRateManagerMode value);
	void setBevelMode(BevelMode value);
	void setBevelShadingMode(BevelShadingMode value);
	void setBevelLodDistance(float value);
	void setAntialiasingMode(AntialiasingMode value);
	void setShadowMode(ShadowMode value);
	GraphicsMode getLatchedGraphicsMode();

	GraphicsMode getGraphicsMode() const { return m_graphicsMode; }
	ShadowMode getShadowMode() const { return m_shadowMode; }
	FrameRateManagerMode getFrameRateManagerMode() const { return m_frameRateManagerMode; }

	GraphicsMode m_graphicsMode;
	BevelMode m_bevelMode;
	BevelShadingMode m_bevelShadingMode;
	float m_bevelLodDistance;
	AntialiasingMode m_antialiasingMode;
	ShadowMode m_shadowMode;
	FrameRateManagerMode m_frameRateManagerMode;
	float m_minShadingQuality;
	float m_maxShadingQuality;
	float m_minMeshDetail;
	float m_maxMeshDetail;
	bool m_delaySwapBuffer;
	std::string m_profileName;
	G3D::Vector2int16 m_fullscreenSize;
	G3D::Vector2int16 m_windowSize;
};

extern CRenderSettings::GraphicsMode latchedGraphicsMode;

inline CRenderSettings::GraphicsMode CRenderSettings::getLatchedGraphicsMode()
{
	if (latchedGraphicsMode == 0) {
		latchedGraphicsMode = m_graphicsMode;
	}
	return latchedGraphicsMode;
}

extern RBX::Reflection::BoundProp<bool, 1> prop_delaySwapBuffer;

extern CRenderSettings::AASamples aaSamples;

#endif // RENDERSETTINGS_H
