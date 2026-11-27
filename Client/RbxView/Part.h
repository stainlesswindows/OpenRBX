#ifndef RBXVIEW_PART_H
#define RBXVIEW_PART_H

#include "renderlib/Chunk.h"
#include "renderlib/Material.h"
#include "renderlib/Mesh.h"
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

namespace View {

class View;

float primaryComponent(const G3D::Vector3& v);

class PartChunk : public Render::Chunk
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

protected:
	std::shared_ptr<PartInstance> partInstance;
	G3D::ReferenceCountedPointer<Render::Material> material;
	bool materialInvalid;
	G3D::ReferenceCountedPointer<Render::Mesh> mesh;
	unsigned int bevelGeneration;
	View* view;
	SpecialShape* specialShape;

	PartChunk(float polygonOffset, const std::shared_ptr<PartInstance>& partInstance, View* view);

	void invalidateMaterial();
	void invalidateMesh();

	virtual void onPropertyChanged(const Reflection::PropertyDescriptor* descriptor);

	G3D::ReferenceCountedPointer<Render::Mesh> getMesh() override;
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

	G3D::ReferenceCountedPointer<Render::Material> getMaterial() override;

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

	G3D::ReferenceCountedPointer<Render::Material> getMaterial() override;

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

	G3D::ReferenceCountedPointer<Render::Material> getMaterial() override;

protected:
	void onTexturePropertyChanged(const Reflection::PropertyDescriptor* descriptor);
	void onTextureAncestorChanged(std::shared_ptr<Instance> ancestor);

private:
	void updateMesh() override;
};

} // namespace View
} // namespace RBX

#endif // RBXVIEW_PART_H
