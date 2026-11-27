#ifndef RBXVIEW_VIEW_H
#define RBXVIEW_VIEW_H

#include "ViewBase.h"
#include "renderlib/AggregatingSceneManager.h"
#include "util/Signals.h"

#include <G3D/ReferenceCount.h>
#include <map>
#include <memory>

namespace G3D {
class Sky;
class TextureManager;
} // namespace G3D

namespace RBX {

class DataModel;
class Instance;

namespace Render {
class Material;
class Model;
class RenderScene;
} // namespace Render

namespace View {

class MaterialFactory;

void setBevelsEnabled(bool value);
void setBevelSmoothShading(bool value);
void setBevelLodDistance(float value);
float getBevelLodDistance();
bool getBevelsEnabled();
unsigned int getBevelGeneration();

class View : public ViewBase
{
private:
	G3D::ReferenceCountedPointer<G3D::Sky> sky;
	std::shared_ptr<DataModel> dataModel;
	std::map<Instance*, Render::Model*> models;
	Signals::ScopedConnection lightingChangedConnection;
	Signals::ScopedConnection workspaceDescendentAddedConnection;
	bool lightingValid;

public:
	std::unique_ptr<Render::SceneManager> sceneManager;
	std::unique_ptr<Render::RenderScene> renderLibScene;
	std::unique_ptr<G3D::TextureManager> textureManager;
	std::unique_ptr<MaterialFactory> materialFactory;

	View(std::shared_ptr<DataModel> dataModel);
	~View() override;

	void render(void* rd) override;

	G3D::ReferenceCountedPointer<Render::Material> getMaterial(G3D::ReferenceCountedPointer<Render::Material>);

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
};

} // namespace View
} // namespace RBX

#endif // RBXVIEW_VIEW_H
