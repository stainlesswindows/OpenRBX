#ifndef RENDERLIBNEW_PART_H
#define RENDERLIBNEW_PART_H

#include "renderlibnew/Chunk.h"
#include "renderlibnew/Material.h"
#include "renderlibnew/Mesh.h"
#include "util/Events.h"
#include "util/Signals.h"
#include "v8world/IMoving.h"

#include <G3D/CoordinateFrame.h>
#include <G3D/ReferenceCount.h>
#include <G3D/Vector3.h>
#include <memory>

namespace RBX {

class Decal;
class Instance;
class PartInstance;
class SpecialShape;
class Texture;

namespace Reflection {
class PropertyDescriptor;
}

namespace ViewNew {

class View;

float primaryComponent(const G3D::Vector3& v);

class PartChunk : public RenderNew::Chunk
{
private:
	G3D::CoordinateFrame coordinateFrame;

	virtual void updateMesh() = 0;

	Signals::ScopedConnection childAddedConnection;
	Signals::ScopedConnection childRemovedConnection;
	Signals::ScopedConnection ancestorChangedConnection;
	Signals::ScopedConnection propertyChangedConnection;
	Signals::ScopedConnection shapePropertyChangedConnection;

public:
	~PartChunk() override;

	bool castsShadows() const override;
	bool cullable() const override;
	static void invalidateAllMeshes();

protected:
	std::shared_ptr<PartInstance> partInstance;
	G3D::ReferenceCountedPointer<RenderNew::Material> material;
	bool materialInvalid;
	G3D::ReferenceCountedPointer<RenderNew::Mesh> mesh;
	View* view;
	SpecialShape* specialShape;

	PartChunk(float polygonOffset, const std::shared_ptr<PartInstance>& partInstance, View* view);

	void invalidateMaterial();
	void invalidateMesh();

	virtual void onPropertyChanged(const Reflection::PropertyDescriptor* descriptor);

	G3D::ReferenceCountedPointer<RenderNew::Mesh> getMesh() override;
	const G3D::CoordinateFrame& cframe() override;

private:
	void onAncestorChanged(std::shared_ptr<Instance> ancestor);
	void onChildAdded(std::shared_ptr<Instance> child);
	void onChildRemoved(std::shared_ptr<Instance> child);
	void onSpecialShapeChanged();
};

class Part : public PartChunk, public Listener<PartInstance, CanAggregateChanged>
{
public:
	Part(const std::shared_ptr<PartInstance>& partInstance, View* view);

	~Part() override;

	G3D::ReferenceCountedPointer<RenderNew::Material> getMaterial() override;

protected:
	void onPropertyChanged(const Reflection::PropertyDescriptor* descriptor) override;
	void onEvent(const PartInstance* source, CanAggregateChanged event) override;

private:
	void updateMesh() override;
};

class Decal : public PartChunk
{
private:
	std::shared_ptr<RBX::Decal> decal;
	Signals::ScopedConnection decalAncestorChangedConnection;
	Signals::ScopedConnection decalPropertyChangedConnection;

public:
	Decal(RBX::Decal& decal, PartInstance& parentPart, View* view);

	~Decal() override;

	G3D::ReferenceCountedPointer<RenderNew::Material> getMaterial() override;

protected:
	void onDecalPropertyChanged(const Reflection::PropertyDescriptor* descriptor);
	void onDecalAncestorChanged(std::shared_ptr<Instance> ancestor);

private:
	void updateMesh() override;
};

class Texture : public PartChunk
{
private:
	std::shared_ptr<RBX::Texture> texture;
	Signals::ScopedConnection textureAncestorChangedConnection;
	Signals::ScopedConnection texturePropertyChangedConnection;

public:
	Texture(RBX::Texture& texture, PartInstance& parentPart, View* view);

	~Texture() override;

	G3D::ReferenceCountedPointer<RenderNew::Material> getMaterial() override;

protected:
	void onTexturePropertyChanged(const Reflection::PropertyDescriptor* descriptor);
	void onTextureAncestorChanged(std::shared_ptr<Instance> ancestor);

private:
	void updateMesh() override;
};

} // namespace ViewNew
} // namespace RBX

#endif // RENDERLIBNEW_PART_H
