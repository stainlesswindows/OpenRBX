#include "View.h"

#include "BevelMesh.h"
#include "BrickMesh.h"
#include "CylinderMesh.h"
#include "MaterialFactory.h"
#include "PBBMesh.h"
#include "Part.h"
#include "SphereMesh.h"
#include "TorsoMesh.h"
#include "WedgeMesh.h"
#include "appdraw/RbxSceneManager.h"
#include "appdraw/RbxSky.h"
#include "appdraw/VisualEngine.h"
#include "renderlibnew/AggregatingSceneManager.h"
#include "util/ContentProvider.h"
#include "util/standardout.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/Lighting.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Sky.h"
#include "v8datamodel/Workspace.h"

#include <G3D/GImage.h>
#include <GLG3D/TextureManager.h>
#include <OgreDataStream.h>
#include <OgreException.h>
#include <OgreLogManager.h>
#include <OgreMaterialManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreStringConverter.h>
#include <cstdio>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace RBX::ViewNew {

char skyBoxMaterialScript[] =
	"material %s\n{\n\ttechnique\n\t{\n\t\tpass\n\t\t{\n\t\t\tlighting off\n\t\t\tdepth_write "
	"off\n\t\t\ttexture_unit\n\t\t\t{\n\t\t\t\tcubic_texture %s %s %s %s %s %s "
	"separateUV\n\t\t\t\ttex_address_mode clamp\n\t\t\t}\n\t\t}\n\t}\n}\n";

const char* imageFormatSuffixes[] = {"JPG", "BMP", "TGA", "PCX", "ICO", "PNG", "PPM", "PPM"};

void View::invalidateLighting(bool updateSkybox)
{
	lightingValid = false;
	if (updateSkybox) {
		m_skyBoxValid = false;
	}
}

bool bevelsEnabled = true;
static bool bevelChangePending = false;
static float bevelLodDistance = 80.0f;

void setBevelsEnabled(bool value)
{
	if (bevelsEnabled == value) {
		return;
	}

	bevelsEnabled = value;
	brickMeshBevels = value;
	bevelChangePending = true;
}

void setBevelSmoothShading(bool value)
{
	if (!BevelMesh::setSmoothShadingEnabled(value)) {
		return;
	}

	bevelChangePending = true;
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

Instance* View::getWorkspace()
{
	return dataModel->getWorkspace();
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

	G3D::Color4 clearColor = lighting->getClearColor();
	renderLibScene->colorClearValue = clearColor;
}

void View::loadSkyBox(void* rd)
{
	Ogre::VisualEngine* engine = static_cast<Ogre::VisualEngine*>(rd);
	std::string files[6];
	Lighting* lighting = dataModel ? dataModel->create<Lighting>() : nullptr;

	ContentId up = ContentId();
	ContentId lf = ContentId();
	ContentId rt = ContentId();
	ContentId bk = ContentId();
	ContentId ft = ContentId();
	ContentId dn = ContentId();
	if (lighting->sky == nullptr) {
		up = ContentId::fromAssets("Sky\\null_plainsky512_up.jpg");
		lf = ContentId::fromAssets("Sky\\null_plainsky512_lf.jpg");
		rt = ContentId::fromAssets("Sky\\null_plainsky512_rt.jpg");
		bk = ContentId::fromAssets("Sky\\null_plainsky512_bk.jpg");
		ft = ContentId::fromAssets("Sky\\null_plainsky512_ft.jpg");
		dn = ContentId::fromAssets("Sky\\null_plainsky512_dn.jpg");
	}
	else {
		up = lighting->sky->skyUp;
		dn = lighting->sky->skyDn;
		lf = lighting->sky->skyLf;
		rt = lighting->sky->skyRt;
		ft = lighting->sky->skyFt;
		bk = lighting->sky->skyBk;
	}

	std::string skyFolder =
		ContentProvider::singleton().getAssetFile("Sky") + (char) std::filesystem::path::preferred_separator;
	try {
		files[0] = ContentProvider::singleton().getFile(ft);
		files[1] = ContentProvider::singleton().getFile(bk);
		files[2] = ContentProvider::singleton().getFile(lf);
		files[3] = ContentProvider::singleton().getFile(rt);
		files[4] = ContentProvider::singleton().getFile(up);
		files[5] = ContentProvider::singleton().getFile(dn);
	}
	catch (...) {
		files[0] = skyFolder + "null_plainsky512_ft.jpg";
		files[1] = skyFolder + "null_plainsky512_bk.jpg";
		files[2] = skyFolder + "null_plainsky512_lf.jpg";
		files[3] = skyFolder + "null_plainsky512_rt.jpg";
		files[4] = skyFolder + "null_plainsky512_up.jpg";
		files[5] = skyFolder + "null_plainsky512_dn.jpg";
	}

	std::string moved[6];
	std::string baseName;
	std::string path;
	Ogre::StringUtil::splitFilename(files[0], baseName, path);
	if (baseName.find(".") == std::string::npos) {
		G3D::GImage::Format format = G3D::GImage::resolveFormat(files[0]);
		std::string suffix;
		if (format >= 0 && format < sizeof(imageFormatSuffixes) / sizeof(imageFormatSuffixes[0])) {
			suffix = std::string(".") + imageFormatSuffixes[format];
		}
		else {
			suffix = ".raw";
		}

		for (int i = 0; i < 6; i++) {
			moved[i] = files[i] + suffix;
#ifdef _WIN32
			MoveFileEx(files[i].c_str(), moved[i].c_str(), MOVEFILE_REPLACE_EXISTING);
#else
			std::rename(files[i].c_str(), moved[i].c_str());
#endif
			files[i] = moved[i];
		}
	}

	std::string shortNames[6];
	for (int i = 0; i < 6; i++) {
#ifdef _WIN32
		char shortName[0x400];
		GetShortPathName(files[i].c_str(), shortName, sizeof(shortName));
		shortNames[i] = shortName;
#else
		shortNames[i] = files[i];
#endif
	}

	char script[0x800];
	snprintf(
		script,
		sizeof(script),
		skyBoxMaterialScript,
		"rbxskybox",
		shortNames[1].c_str(),
		shortNames[0].c_str(),
		shortNames[2].c_str(),
		shortNames[3].c_str(),
		shortNames[4].c_str(),
		shortNames[5].c_str()
	);

	Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(script, strlen(script) + 1));
	if (Ogre::MaterialManager::getSingleton().resourceExists("rbxskybox")) {
		Ogre::MaterialManager::getSingleton().remove("rbxskybox");
	}
	try {
		Ogre::MaterialManager::getSingleton().parseScript(stream, "General");
		Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName("rbxskybox");

		Ogre::RbxSceneManager* sceneManager = static_cast<Ogre::RbxSceneManager*>(engine->m_sceneManager);
		sceneManager->m_skyFolder = skyFolder;
		engine->m_sceneManager->setSkyBox(
			true,
			"rbxskybox",
			2700.0f,
			true,
			Ogre::Quaternion::IDENTITY,
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
		);
	}
	catch (Ogre::Exception& e) {
		Ogre::LogManager::getSingleton().logMessage(std::string("RBX error in skybox: ") + e.what());
	}

	if (moved[0] != "") {
		for (int i = 0; i < 6; i++) {
#ifdef _WIN32
			MoveFileEx(moved[i].c_str(), files[i].c_str(), MOVEFILE_REPLACE_EXISTING);
#else
			std::rename(moved[i].c_str(), files[i].c_str());
#endif
		}
	}
}

void View::updateLighting(void* rd)
{
	if (!lightingValid) {
		Ogre::VisualEngine* engine = static_cast<Ogre::VisualEngine*>(rd);
		Lighting* lighting = dataModel ? dataModel->create<Lighting>() : nullptr;

		if (lighting->isSkySuppressed()) {
			engine->m_sceneManager->setSkyBox(
				false,
				"",
				5000.0f,
				true,
				Ogre::Quaternion::IDENTITY,
				Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
			);
		}
		else if (!m_skyBoxValid) {
			try {
				loadSkyBox(rd);
				m_skyBoxValid = true;
			}
			catch (G3D::GImage::Error& e) {
				StandardOut::singleton()->print(MESSAGE_ERROR, "Failed to load sky box. %s", e.reason.c_str());
			}
			catch (...) {
				StandardOut::singleton()->print(MESSAGE_ERROR, "Failed to load sky box");
			}
		}

		renderLibScene->presetLighting(
			G3D::ReferenceCountedPointer<G3D::Sky>(),
			lighting->getSkyParameters(),
			lighting->getAmbientTop(),
			lighting->getAmbientBottom()
		);
		G3D::Color4 clearColor = lighting->getClearColor();
		renderLibScene->colorClearValue = clearColor;
		static_cast<Ogre::RbxSceneManager*>(engine->m_sceneManager)
			->m_sky->update(engine, lighting->getSkyParameters());
		lightingValid = true;
	}
}

void View::render(void* rd)
{
	updateLighting(rd);
	if (bevelChangePending) {
		PartChunk::invalidateAllMeshes();
		MeshFactory<PBBMesh, 4>::flushCache();
		MeshFactory<WedgeMesh, 1>::flushCache();
		MeshFactory<TorsoMesh, 1>::flushCache();
		bevelChangePending = false;
	}
	sceneManager->prerender(0.0);
	renderLibScene->render(static_cast<G3D::RenderDevice*>(rd), dataModel->getWorkspace()->getGCamera());
}

View::View(std::shared_ptr<DataModel> dataModel)
	: dataModel(dataModel), lightingValid(false), renderLibScene(new RenderNew::RenderScene()),
	  textureManager(new G3D::TextureManager()), m_skyBoxValid(false)
{
	sceneManager.reset(new RenderNew::AggregatingSceneManager(renderLibScene.get()));

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

void View::updateLighting()
{
}

} // namespace RBX::ViewNew
