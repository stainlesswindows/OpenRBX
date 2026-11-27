#include "Part.h"

#include "CylinderMesh.h"
#include "FileMesh.h"
#include "HeadMesh.h"
#include "MaterialFactory.h"
#include "MeshFactory.h"
#include "PBBMesh.h"
#include "SphereMesh.h"
#include "TorsoMesh.h"
#include "WedgeMesh.h"
#include "rbxview/View.h"
#include "renderlib/TextureProxy.h"
#include "util/ContentProvider.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/custommesh.h"

namespace RBX::View {

bool PartChunk::castsShadows() const
{
	return true;
}

bool PartChunk::cullable() const
{
	return true;
}

float primaryComponent(const G3D::Vector3& v)
{
	float nx = fabs(v.x);
	float ny = fabs(v.y);
	float nz = fabs(v.z);

	if (nx > ny) {
		if (nx > nz) {
			return nx;
		}

		return nz;
	}

	if (ny > nz) {
		return ny;
	}

	return nz;
}

const G3D::CoordinateFrame& PartChunk::cframe()
{
	coordinateFrame = partInstance->getCoordinateFrame();

	return coordinateFrame;
}

G3D::ReferenceCountedPointer<Render::Material> Part::getMaterial()
{
	try {
		if (materialInvalid) {
			MaterialFactory::Attributes attr = {
				partInstance->getColor(), partInstance->getTransparencyUi(), partInstance->getReflectance()
			};

			if (specialShape && specialShape->getMeshType() == SpecialShape::FILE_MESH) {
				std::string textureFile;
				if (!ContentProvider::singleton().requestContentFile(specialShape->getTextureId(), textureFile)) {
					material = view->materialFactory->getMaterial(attr);
					return material;
				}

				G3D::Vector3 color = specialShape->getVertColor();
				float trans = specialShape->getAlpha();

				material = new Render::Material();
				material->appendLevel(
					new Render::TextureProxy(*view->textureManager, textureFile, false),
					G3D::Color3(color),
					0.0f,
					0.1f,
					0.0f,
					trans
				);
				materialInvalid = false;
			}
			else {
				material = view->materialFactory->getMaterial(attr);
				materialInvalid = false;
			}
		}
	}
	catch (...) {
	}

	return material;
}

void Part::onEvent(const PartInstance* source, CanAggregateChanged event)
{
	view->sceneManager->setSleeping(this, event.canClump);
}

void PartChunk::onAncestorChanged(std::shared_ptr<Instance> ancestor)
{
	Workspace* workspace = Workspace::findWorkspace(partInstance.get());
	Instance* parent;

	if (workspace == nullptr || (workspace != (parent = partInstance->getParent()) &&
								 (parent == nullptr || !parent->isDescendentOf(workspace)))) {
		view->sceneManager->removeModel(this);
	}
}

void Decal::onDecalAncestorChanged(std::shared_ptr<Instance> ancestor)
{
	Workspace* workspace = Workspace::findWorkspace(decal.get());
	Instance* parent;

	if (workspace == nullptr ||
		(workspace != (parent = decal->getParent()) && (parent == nullptr || !parent->isDescendentOf(workspace)))) {
		view->sceneManager->removeModel(this);
	}
}

void Texture::onTextureAncestorChanged(std::shared_ptr<Instance> ancestor)
{
	Workspace* workspace = Workspace::findWorkspace(texture.get());
	Instance* parent;

	if (workspace == nullptr ||
		(workspace != (parent = texture->getParent()) && (parent == nullptr || !parent->isDescendentOf(workspace)))) {
		view->sceneManager->removeModel(this);
	}
}

void PartChunk::invalidateMesh()
{
	mesh = nullptr;

	view->sceneManager->invalidateModel(this, partInstance->getCanAggregate());
}

void PartChunk::invalidateMaterial()
{
	material = nullptr;
	materialInvalid = true;

	view->sceneManager->invalidateModel(this, partInstance->getCanAggregate());
}

void PartChunk::onSpecialShapeChanged()
{
	invalidateMesh();
	invalidateMaterial();
}

void PartChunk::onPropertyChanged(const Reflection::PropertyDescriptor* descriptor)
{
	if (descriptor == &PartInstance::prop_Size) {
		invalidateMesh();
		invalidateMaterial();
	}
	else if (descriptor == &PartInstance::prop_shapeXml) {
		invalidateMesh();
	}
	else if (Surfaces::isSurfaceDescriptor(*descriptor)) {
		invalidateMesh();
	}
}

void Part::onPropertyChanged(const Reflection::PropertyDescriptor* descriptor)
{
	PartChunk::onPropertyChanged(descriptor);

	if (descriptor == &PartInstance::prop_Color) {
		invalidateMaterial();
	}
	else if (descriptor == &PartInstance::prop_Transparency || descriptor == &PartInstance::prop_Reflectance) {
		invalidateMaterial();
	}
}

G3D::ReferenceCountedPointer<Render::Mesh> PartChunk::getMesh()
{
	if (mesh.isNull() || bevelGeneration != RBX::View::getBevelGeneration()) {
		updateMesh();
		bevelGeneration = RBX::View::getBevelGeneration();
	}

	return mesh;
}

void Decal::onDecalPropertyChanged(const Reflection::PropertyDescriptor* descriptor)
{
	if (descriptor == &FaceInstance::prop_Face) {
		invalidateMesh();
	}
	else if (descriptor == &RBX::Decal::prop_Texture) {
		invalidateMaterial();
	}
	else if (descriptor == &RBX::Decal::prop_Specular) {
		invalidateMaterial();
	}
	else if (descriptor == &RBX::Decal::prop_Shiny) {
		invalidateMaterial();
	}
}

G3D::ReferenceCountedPointer<Render::Material> Decal::getMaterial()
{
	try {
		if (materialInvalid) {
			std::string textureFile;
			if (ContentProvider::singleton().requestContentFile(decal->getTexture(), textureFile)) {
				material = new Render::Material();
				material->appendLevel(
					new Render::TextureProxy(*view->textureManager, textureFile, false),
					G3D::Color3::WHITE,
					decal->getSpecular(),
					decal->getShiny(),
					0.0f,
					0.0f
				);
				materialInvalid = false;
			}
		}
	}
	catch (...) {
	}

	return material;
}

void Texture::onTexturePropertyChanged(const Reflection::PropertyDescriptor* descriptor)
{
	if (descriptor == &FaceInstance::prop_Face) {
		invalidateMesh();
	}
	else if (descriptor == &RBX::Decal::prop_Texture) {
		invalidateMaterial();
	}
	else if (descriptor == &RBX::Decal::prop_Specular) {
		invalidateMaterial();
	}
	else if (descriptor == &RBX::Decal::prop_Shiny) {
		invalidateMaterial();
	}
	else if (descriptor == &RBX::Texture::prop_StudsPerTileU) {
		invalidateMesh();
	}
	else if (descriptor == &RBX::Texture::prop_StudsPerTileV) {
		invalidateMesh();
	}
}

G3D::ReferenceCountedPointer<Render::Material> Texture::getMaterial()
{
	try {
		if (materialInvalid) {
			std::string textureFile;
			if (ContentProvider::singleton().requestContentFile(texture->getTexture(), textureFile)) {
				material = new Render::Material();
				material->appendLevel(
					new Render::TextureProxy(*view->textureManager, textureFile, true),
					G3D::Color3::WHITE,
					texture->getSpecular(),
					texture->getShiny(),
					0.0f,
					0.0f
				);
				materialInvalid = false;
			}
		}
	}
	catch (...) {
	}

	return material;
}

void PartChunk::onChildRemoved(std::shared_ptr<Instance> child)
{
	if (child.get() == specialShape) {
		shapePropertyChangedConnection.disconnect();
		specialShape = nullptr;
		invalidateMesh();
	}
}

PartChunk::~PartChunk() = default;

Decal::~Decal() = default;

Texture::~Texture() = default;

Part::~Part()
{
	if (partInstance) {
		partInstance->Notifier<PartInstance, CanAggregateChanged>::removeListener(this);
	}
}

static RenderSurfaceType renderSurfaceType(SurfaceType surfaceType)
{
	switch (surfaceType) {
	case RBX::STUDS:
		return STUDS;
	case RBX::INLET:
		return INLET;
	case RBX::WELD:
		return WELD;
	case RBX::GLUE:
		return GLUE;
	default:
		return FLAT;
	}
}

void Part::updateMesh()
{
	RenderSurfaceTypes surfaceTypes;
	surfaceTypes.setSurfaceType(NORM_X, renderSurfaceType(partInstance->getSurfaces()[NORM_X].getSurfaceType()));
	surfaceTypes.setSurfaceType(NORM_Y, renderSurfaceType(partInstance->getSurfaces()[NORM_Y].getSurfaceType()));
	surfaceTypes.setSurfaceType(NORM_Z, renderSurfaceType(partInstance->getSurfaces()[NORM_Z].getSurfaceType()));
	surfaceTypes.setSurfaceType(
		NORM_X_NEG, renderSurfaceType(partInstance->getSurfaces()[NORM_X_NEG].getSurfaceType())
	);
	surfaceTypes.setSurfaceType(
		NORM_Y_NEG, renderSurfaceType(partInstance->getSurfaces()[NORM_Y_NEG].getSurfaceType())
	);
	surfaceTypes.setSurfaceType(
		NORM_Z_NEG, renderSurfaceType(partInstance->getSurfaces()[NORM_Z_NEG].getSurfaceType())
	);

	G3D::Vector3 size = partInstance->getPartSizeXml();

	if (!specialShape) {
		switch (partInstance->getPartType()) {
		case RBX::Part::BALL_PART:
			mesh = MeshFactory<SphereMesh, 1>::create(size, surfaceTypes);
			break;
		case RBX::Part::BLOCK_PART:
			mesh = MeshFactory<PBBMesh, 4>::create(size, surfaceTypes);
			break;
		case RBX::Part::CYLINDER_PART:
			mesh = MeshFactory<CylinderAlongXMesh, 1>::create(size, surfaceTypes);
			break;
		}
	}
	else {
		size *= specialShape->getScale();

		switch (specialShape->getMeshType()) {
		case SpecialShape::WEDGE_MESH:
			mesh = MeshFactory<WedgeMesh, 1>::create(size, surfaceTypes);
			break;
		case SpecialShape::HEAD_MESH:
			mesh = MeshFactory<HeadMesh, 1>::create(size, surfaceTypes);
			break;
		case SpecialShape::TORSO_MESH:
			mesh = MeshFactory<TorsoMesh, 1>::create(size, surfaceTypes);
			break;
		case SpecialShape::SPHERE_MESH:
			mesh = MeshFactory<SphereMesh, 1>::create(size, surfaceTypes);
			break;
		case SpecialShape::CYLINDER_MESH:
			mesh = MeshFactory<CylinderAlongXMesh, 1>::create(size, surfaceTypes);
			break;
		case SpecialShape::BRICK_MESH:
			mesh = MeshFactory<PBBMesh, 4>::create(size, surfaceTypes);
			break;
		case SpecialShape::FILE_MESH:
			mesh = FileMesh::create(specialShape->getScale(), specialShape->getMeshId());
			break;
		}
	}

	radius = sqrt(2.0f) * 0.5 * primaryComponent(size);
}

void Decal::updateMesh()
{
	if (partInstance) {
		G3D::Vector3 size = partInstance->getPartSizeXml();

		if (specialShape) {
			size *= specialShape->getScale();
		}

		if (!specialShape) {
			switch (partInstance->getPartType()) {
			case RBX::Part::BALL_PART:
				mesh = MeshFactory<SphereMesh, 1>::createDecal(size, decal->getFace());
				break;
			case RBX::Part::BLOCK_PART:
				mesh = MeshFactory<PBBMesh, 4>::createDecal(size, decal->getFace());
				break;
			case RBX::Part::CYLINDER_PART:
				mesh = MeshFactory<CylinderAlongXMesh, 1>::createDecal(size, decal->getFace());
				break;
			}
		}
		else {
			switch (specialShape->getMeshType()) {
			case SpecialShape::WEDGE_MESH:
				mesh = MeshFactory<WedgeMesh, 1>::createDecal(size, decal->getFace());
				break;
			case SpecialShape::HEAD_MESH:
				mesh = MeshFactory<HeadMesh, 1>::createDecal(size, decal->getFace());
				break;
			case SpecialShape::TORSO_MESH:
				mesh = MeshFactory<TorsoMesh, 1>::createDecal(size, decal->getFace());
				break;
			case SpecialShape::SPHERE_MESH:
				mesh = MeshFactory<SphereMesh, 1>::createDecal(size, decal->getFace());
				break;
			case SpecialShape::CYLINDER_MESH:
				mesh = MeshFactory<CylinderAlongXMesh, 1>::createDecal(size, decal->getFace());
				break;
			case SpecialShape::BRICK_MESH:
				mesh = MeshFactory<PBBMesh, 4>::createDecal(size, decal->getFace());
				break;
			}
		}

		radius = sqrt(2.0f) * 0.5 * primaryComponent(size);
	}
}

void Texture::updateMesh()
{
	if (partInstance) {
		G3D::Vector3 size = partInstance->getPartSizeXml();

		if (specialShape) {
			size *= specialShape->getScale();
		}

		if (!specialShape) {
			switch (partInstance->getPartType()) {
			case RBX::Part::BALL_PART:
				mesh = MeshFactory<SphereMesh, 1>::createTexture(size, texture->getFace(), texture->getStudsPerTile());
				break;
			case RBX::Part::BLOCK_PART:
				mesh = MeshFactory<PBBMesh, 4>::createTexture(size, texture->getFace(), texture->getStudsPerTile());
				break;
			case RBX::Part::CYLINDER_PART:
				mesh = MeshFactory<CylinderAlongXMesh, 1>::createTexture(
					size, texture->getFace(), texture->getStudsPerTile()
				);
				break;
			}
		}
		else {
			switch (specialShape->getMeshType()) {
			case SpecialShape::WEDGE_MESH:
				mesh = MeshFactory<WedgeMesh, 1>::createTexture(size, texture->getFace(), texture->getStudsPerTile());
				break;
			case SpecialShape::HEAD_MESH:
				mesh = MeshFactory<HeadMesh, 1>::createTexture(size, texture->getFace(), texture->getStudsPerTile());
				break;
			case SpecialShape::TORSO_MESH:
				mesh = MeshFactory<TorsoMesh, 1>::createTexture(size, texture->getFace(), texture->getStudsPerTile());
				break;
			case SpecialShape::SPHERE_MESH:
				mesh = MeshFactory<SphereMesh, 1>::createTexture(size, texture->getFace(), texture->getStudsPerTile());
				break;
			case SpecialShape::CYLINDER_MESH:
				mesh = MeshFactory<CylinderAlongXMesh, 1>::createTexture(
					size, texture->getFace(), texture->getStudsPerTile()
				);
				break;
			case SpecialShape::BRICK_MESH:
				mesh = MeshFactory<PBBMesh, 4>::createTexture(size, texture->getFace(), texture->getStudsPerTile());
				break;
			}
		}

		radius = sqrt(2.0f) * 0.5 * primaryComponent(size);
	}
}

void PartChunk::onChildAdded(std::shared_ptr<Instance> child)
{
	SpecialShape* specialShape = dynamic_cast<SpecialShape*>(child.get());

	if (specialShape != nullptr) {
		this->specialShape = specialShape;

		shapePropertyChangedConnection = Instance::event_propertyChanged.connect(
			specialShape, [this](const Reflection::PropertyDescriptor*) { onSpecialShapeChanged(); }, 10
		);

		invalidateMesh();
	}
}

PartChunk::PartChunk(float polygonOffset, const std::shared_ptr<PartInstance>& partInstance, View* view)
	: Render::Chunk(polygonOffset), partInstance(partInstance), bevelGeneration(RBX::View::getBevelGeneration()),
	  materialInvalid(true), view(view), specialShape(nullptr)
{
	view->sceneManager->addModel(this);

	childAddedConnection = Instance::event_childAdded.connect(
		partInstance.get(), [this](const std::shared_ptr<Instance>& child) { onChildAdded(child); }, 10
	);
	childRemovedConnection = Instance::event_childRemoved.connect(
		partInstance.get(), [this](const std::shared_ptr<Instance>& child) { onChildRemoved(child); }, 10
	);
	ancestorChangedConnection = Instance::event_ancestryChanged.connect(
		partInstance.get(), [this](const std::shared_ptr<Instance>& ancestor) { onAncestorChanged(ancestor); }, 10
	);
	propertyChangedConnection = Instance::event_propertyChanged.connect(
		partInstance.get(),
		[this](const Reflection::PropertyDescriptor* descriptor) { onPropertyChanged(descriptor); },
		10
	);

	partInstance->visitChildren([this](const std::shared_ptr<Instance>& child) { onChildAdded(child); });
}

Decal::Decal(RBX::Decal& decal, PartInstance& parentPart, View* view)
	: PartChunk(Render::Chunk::DECAL_OFFSET, shared_from(&parentPart), view), decal(shared_from(&decal))
{
	decalAncestorChangedConnection = Instance::event_ancestryChanged.connect(
		&decal, [this](const std::shared_ptr<Instance>& ancestor) { onDecalAncestorChanged(ancestor); }, 10
	);
	decalPropertyChangedConnection = Instance::event_propertyChanged.connect(
		&decal, [this](const Reflection::PropertyDescriptor* descriptor) { onDecalPropertyChanged(descriptor); }, 10
	);
}

Texture::Texture(RBX::Texture& texture, PartInstance& parentPart, View* view)
	: PartChunk(Render::Chunk::TEXTURE_OFFSET, shared_from(&parentPart), view), texture(shared_from(&texture))
{
	textureAncestorChangedConnection = Instance::event_ancestryChanged.connect(
		&texture, [this](const std::shared_ptr<Instance>& ancestor) { onTextureAncestorChanged(ancestor); }, 10
	);
	texturePropertyChangedConnection = Instance::event_propertyChanged.connect(
		&texture, [this](const Reflection::PropertyDescriptor* descriptor) { onTexturePropertyChanged(descriptor); }, 10
	);
}

Part::Part(const std::shared_ptr<PartInstance>& partInstance, View* view) : PartChunk(0.0f, partInstance, view)
{
	if (partInstance->getCanAggregate()) {
		view->sceneManager->setSleeping(this, true);
	}
	startListening(partInstance, this);
	invalidateMesh();
	invalidateMaterial();
}

} // namespace RBX::View
