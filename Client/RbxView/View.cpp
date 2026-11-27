#include "rbxview/View.h"

#include "BevelMesh.h"
#include "CylinderMesh.h"
#include "MaterialFactory.h"
#include "PBBMesh.h"
#include "Part.h"
#include "SphereMesh.h"
#include "TorsoMesh.h"
#include "WedgeMesh.h"
#include "renderlib/RenderScene.h"
#include "util/standardout.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/Lighting.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Sky.h"
#include "v8datamodel/Workspace.h"

#include <GLG3D/Sky.h>

namespace RBX::View {

static bool bevelsEnabled = true;
static unsigned int bevelGeneration = 0;
static float bevelLodDistance = 80.0f;

void setBevelsEnabled(bool value)
{
	if (bevelsEnabled == value) {
		return;
	}

	bevelsEnabled = value;
	++bevelGeneration;
	MeshFactory<PBBMesh, 4>::flushCache();
	MeshFactory<WedgeMesh, 1>::flushCache();
	MeshFactory<TorsoMesh, 1>::flushCache();
}

void setBevelSmoothShading(bool value)
{
	if (!BevelMesh::setSmoothShadingEnabled(value)) {
		return;
	}

	++bevelGeneration;
	MeshFactory<PBBMesh, 4>::flushCache();
	MeshFactory<WedgeMesh, 1>::flushCache();
	MeshFactory<TorsoMesh, 1>::flushCache();
}

void setBevelLodDistance(float value)
{
	bevelLodDistance = value;
}

float getBevelLodDistance()
{
	return bevelLodDistance;
}

bool getBevelsEnabled()
{
	return bevelsEnabled;
}

unsigned int getBevelGeneration()
{
	return bevelGeneration;
}

Instance* View::getWorkspace()
{
	return dataModel->getWorkspace();
}

void View::invalidateLighting(bool updateSkybox)
{
	lightingValid = false;
	if (updateSkybox) {
		sky = nullptr;
	}
}

float View::getShadingQuality() const
{
	return renderLibScene->getShadingQuality();
}

float View::getMeshDetail() const
{
	return renderLibScene->getMeshDetail();
}

void View::updateSettings(float shadingQuality, float meshDetail, bool shadows, float cameraDistance)
{
	renderLibScene->setThrottle(shadingQuality, meshDetail, shadows, cameraDistance);
}

void View::render(void* rd)
{
	updateLighting();
	sceneManager->prerender(0.1f);
	renderLibScene->render(static_cast<G3D::RenderDevice*>(rd), dataModel->getWorkspace()->getGCamera());
}

RenderStats& View::getRenderStats()
{
	return renderLibScene->renderStats;
}

void View::onWorkspaceDescendentAdded(std::shared_ptr<Instance> descendent)
{
	PartInstance* part = dynamic_cast<PartInstance*>(descendent.get());
	if (part) {
		new Part(shared_from(part), this);
	}
	else if (PartInstance* parentPart = dynamic_cast<PartInstance*>(descendent->getParent())) {
		RBX::Texture* texture = dynamic_cast<RBX::Texture*>(descendent.get());
		if (texture) {
			new Texture(*texture, *parentPart, this);
			return;
		}

		RBX::Decal* decal = dynamic_cast<RBX::Decal*>(descendent.get());
		if (decal) {
			new Decal(*decal, *parentPart, this);
			return;
		}
	}
}

void View::suppressSkybox()
{
	Lighting* lighting = dataModel ? dataModel->create<Lighting>() : nullptr;

	renderLibScene->presetLighting(
		nullptr, lighting->getSkyParameters(), lighting->getAmbientTop(), lighting->getAmbientBottom()
	);

	sky = nullptr;

	G3D::Color4 clearColor = lighting->getClearColor();
	renderLibScene->colorClearValue = clearColor;
}

void View::updateLighting()
{
	if (!lightingValid) {
		Lighting* lighting = dataModel ? dataModel->create<Lighting>() : nullptr;

		if (lighting->isSkySuppressed()) {
			sky = nullptr;
		}
		else if (sky.isNull()) {
			try {
				sky = Sky::getG3DSky(lighting->sky.get());
			}
			catch (const G3D::GImage::Error& e) {
				StandardOut::singleton()->print(MESSAGE_ERROR, "Failed to load sky box. %s", e.reason.c_str());
			}
			catch (...) {
				StandardOut::singleton()->print(MESSAGE_ERROR, "Failed to load sky box");
			}
		}

		renderLibScene->presetLighting(
			sky, lighting->getSkyParameters(), lighting->getAmbientTop(), lighting->getAmbientBottom()
		);
		renderLibScene->colorClearValue = lighting->getClearColor();

		lightingValid = true;
	}
}

View::View(std::shared_ptr<DataModel> dataModel)
	: dataModel(dataModel), lightingValid(false), renderLibScene(new Render::RenderScene()),
	  textureManager(new G3D::TextureManager())
{
	sceneManager.reset(new Render::AggregatingSceneManager(renderLibScene.get()));

	materialFactory.reset(new MaterialFactory(textureManager.get()));

	renderLibScene->setThrottle(0, 0, false, 0);

	Lighting* lighting = dataModel ? dataModel->create<Lighting>() : nullptr;
	lightingChangedConnection = Lighting::event_LightingChanged.connect(
		lighting, [this](bool updateSkybox) { invalidateLighting(updateSkybox); }, 10
	);

	dataModel->getWorkspace()->visitDescendents([this](const std::shared_ptr<Instance>& descendent) {
		onWorkspaceDescendentAdded(descendent);
	});
	workspaceDescendentAddedConnection = Instance::event_descendentAdded.connect(
		dataModel->getWorkspace(),
		[this](const std::shared_ptr<Instance>& descendent) { onWorkspaceDescendentAdded(descendent); },
		10
	);
}

View::~View()
{
	sceneManager->clear();

	if (dataModel) {
		dataModel->find<Lighting>();
	}

	MeshFactory<CylinderAlongXMesh, 1>::flushCache();
	MeshFactory<SphereMesh, 1>::flushCache();
	MeshFactory<PBBMesh, 4>::flushCache();
}

} // namespace RBX::View
