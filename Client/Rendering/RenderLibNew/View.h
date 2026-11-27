#ifndef RENDERLIBNEW_VIEW_H
#define RENDERLIBNEW_VIEW_H

#include "ViewBase.h"
#include "renderlibnew/RenderScene.h"
#include "util/Signals.h"

#include <map>
#include <memory>

namespace G3D {
class TextureManager;
} // namespace G3D

namespace RBX {

class DataModel;
class Instance;

namespace RenderNew {
class Model;
} // namespace RenderNew

namespace ViewNew {

class MaterialFactory;

class View : public ViewBase
{
private:
	std::shared_ptr<DataModel> dataModel;
	std::map<Instance*, RenderNew::Model*> models;
	Signals::ScopedConnection lightingChangedConnection;
	Signals::ScopedConnection workspaceDescendentAddedConnection;
	bool lightingValid;

public:
	std::unique_ptr<RenderNew::SceneManager> sceneManager;
	std::unique_ptr<RenderNew::RenderScene> renderLibScene;
	std::unique_ptr<G3D::TextureManager> textureManager;
	std::unique_ptr<MaterialFactory> materialFactory;

private:
	bool m_skyBoxValid;

public:
	View(std::shared_ptr<DataModel> dataModel);
	~View() override;

	void render(void* rd) override;

	float getShadingQuality() const override;
	float getMeshDetail() const override;
	void updateSettings(float shadingQuality, float meshDetail, bool shadows, float cameraDistance) override;
	void suppressSkybox() override;
	Instance* getWorkspace() override;
	RenderStats& getRenderStats() override;

private:
	void onWorkspaceDescendentAdded(std::shared_ptr<Instance> descendent) override;
	void updateLighting() override;
	void invalidateLighting(bool updateSkybox) override;

	void loadSkyBox(void* rd);
	void updateLighting(void* rd);
};

extern bool bevelsEnabled;

void setBevelsEnabled(bool value);
void setBevelSmoothShading(bool value);
void setBevelLodDistance(float value);
float getBevelLodDistance();
bool getBevelsEnabled();

} // namespace ViewNew
} // namespace RBX

#endif // RENDERLIBNEW_VIEW_H
