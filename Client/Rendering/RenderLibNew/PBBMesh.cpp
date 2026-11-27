#include "PBBMesh.h"

#include "BevelMesh.h"
#include "BrickMesh.h"
#include "QuadVolume.h"
#include "ViewBase.h"

#include <G3D/Vector2int16.h>
#include <G3D/Vector3int16.h>
#include <G3D/g3dmath.h>
#include <algorithm>

static float bevelForSize(const G3D::Vector3& size)
{
	return std::min(0.065f, std::min(size.x, std::min(size.y, size.z)) * 0.25f);
}

namespace RBX::ViewNew {

class Face
{
private:
	G3D::Vector2 perturbation;
	bool bulge[2];

public:
	Face();

	void operator()(
		G3D::Vector3& vertex,
		G3D::Vector3& normal,
		G3D::Vector2& tex,
		const G3D::Vector3& radius,
		const G3D::Vector2int16& ivertex
	)
	{
		for (int i = 0; i < 2; ++i) {
			if (bulge[i]) {
				normal[i] = vertex[i] * perturbation[i] / radius[i];
			}
			else {
				normal[i] = perturbation[i];
			}
		}
	}
};

class TopBottom : public Face
{
private:
	const float bevel;

public:
	explicit TopBottom(float bevel) : bevel(bevel) {}

	void operator()(
		G3D::Vector3& vertex,
		G3D::Vector3& normal,
		G3D::Vector2& tex,
		const G3D::Vector3& radius,
		const G3D::Vector2int16& ivertex
	)
	{
		const float inset = BrickMesh::widthInset + bevel;
		if (vertex.x > 0) {
			vertex.x -= inset;
		}
		else {
			vertex.x += inset;
		}
		if (vertex.y > 0) {
			vertex.y -= inset;
		}
		else {
			vertex.y += inset;
		}
		Face::operator()(vertex, normal, tex, radius, ivertex);
	}
};

class Side : public Face
{
private:
	const float bevel;

public:
	explicit Side(float bevel) : bevel(bevel) {}

	void operator()(
		G3D::Vector3& vertex,
		G3D::Vector3& normal,
		G3D::Vector2& tex,
		const G3D::Vector3& radius,
		const G3D::Vector2int16& ivertex
	)
	{
		const float inset = BrickMesh::widthInset + bevel;
		if (vertex.x > 0) {
			vertex.x -= inset;
		}
		else {
			vertex.x += inset;
		}
		if (vertex.y > 0) {
			vertex.y -= bevel;
		}
		else {
			vertex.y += bevel;
		}
		Face::operator()(vertex, normal, tex, radius, ivertex);
	}
};

class PBBBuilder : public BevelMesh::Builder
{
private:
	G3D::Vector3int16 strips;

public:
	PBBBuilder(
		G3D::ReferenceCountedPointer<RBX::RenderNew::Mesh::Level>& level,
		G3D::Vector3 size,
		RenderSurfaceTypes surfaceTypes,
		float bevel
	);

protected:
	int surfaceStripIndex(const G3D::Vector3&) const override { return 1; }
	bool reverseSurfaceU(int) const override { return false; }
	bool reverseSurfaceV(int) const override { return true; }

	void buildTop(Purpose purpose) override;
	void buildBottom(Purpose purpose) override;
	void buildLeft(Purpose purpose) override;
	void buildRight(Purpose purpose) override;
	void buildFront(Purpose purpose) override;
	void buildBack(Purpose purpose) override;
};

PBBBuilder::PBBBuilder(
	G3D::ReferenceCountedPointer<RBX::RenderNew::Mesh::Level>& level,
	G3D::Vector3 size,
	RenderSurfaceTypes surfaceTypes,
	float bevel
)
	: BevelMesh::Builder(level, size, surfaceTypes, bevel), strips(0, 0, 0)
{
	strips.x = std::max(1, (int) (size.x / 6.0));
	strips.y = std::max(1, (int) (size.y / 6.0));
	strips.z = std::max(1, (int) (size.z / 6.0));

	strips.x = std::min(4, (int) strips.x);
	strips.y = std::min(4, (int) strips.y);
	strips.z = std::min(4, (int) strips.z);
}

Face::Face()
{
	perturbation.x = G3D::uniformRandom(-BrickMesh::normalPerturbation, BrickMesh::normalPerturbation);
	perturbation.y = G3D::uniformRandom(-BrickMesh::normalPerturbation, BrickMesh::normalPerturbation);
	bulge[0] = G3D::uniformRandom() > 0.5f;
	bulge[1] = G3D::uniformRandom() > 0.5f;
}

void PBBBuilder::buildTop(Purpose purpose)
{
	buildFace<RBX::NORM_Y>(TopBottom(bevel), 1, 1, purpose);
}

void PBBBuilder::buildBottom(Purpose purpose)
{
	buildFace<RBX::NORM_Y_NEG>(TopBottom(bevel), 1, 1, purpose);
}

void PBBBuilder::buildLeft(Purpose purpose)
{
	buildFace<RBX::NORM_X_NEG>(Side(bevel), 1, 1, purpose);
}

void PBBBuilder::buildRight(Purpose purpose)
{
	buildFace<RBX::NORM_X>(Side(bevel), 1, 1, purpose);
}

void PBBBuilder::buildFront(Purpose purpose)
{
	buildFace<RBX::NORM_Z_NEG>(Side(bevel), 1, 1, purpose);
}

void PBBBuilder::buildBack(Purpose purpose)
{
	buildFace<RBX::NORM_Z>(Side(bevel), 1, 1, purpose);
}

PBBMesh::PBBMesh(const G3D::Vector3& size, NormalId decalFace)
{
	RenderNew::Mesh::LevelRef level = new RenderNew::Mesh::Level(G3D::RenderDevice::QUADS);

	levels.append(level);

	PBBBuilder builder(level, size, RenderSurfaceTypes(), 0.0f);

	builder.buildFace(decalFace, LevelBuilder::Decal);
}

PBBMesh::PBBMesh(const G3D::Vector3& size, NormalId textureFace, const G3D::Vector2& studsPerTile)
{
	RenderNew::Mesh::LevelRef level = new RenderNew::Mesh::Level(G3D::RenderDevice::QUADS);

	levels.append(level);

	PBBBuilder builder(level, size, RenderSurfaceTypes(), 0.0f);

	builder.textureScale = mapToUvw_Legacy(size, textureFace).xy() * 2.0 / studsPerTile;

	builder.buildFace(textureFace, LevelBuilder::Decal);
}

PBBMesh::PBBMesh(const G3D::Vector3& size, RenderSurfaceTypes surfaceTypes)
{
	m_meshType = 1;
	const float bevel = brickMeshBevels ? bevelForSize(size) : 0.0f;

	{
		RenderNew::Mesh::LevelRef level = new RenderNew::Mesh::Level(G3D::RenderDevice::QUADS);
		levels.append(level);
		PBBBuilder builder(level, size, surfaceTypes, bevel);
		builder.build(LevelBuilder::Surface);
	}
	{
		RenderNew::Mesh::LevelRef level = new RenderNew::Mesh::Level(G3D::RenderDevice::QUADS);
		levels.append(level);
		PBBBuilder builder(level, size, surfaceTypes, 0.0f);
		builder.build(LevelBuilder::Surface);
	}

	if (ViewBase::getShadowsEnabled()) {
		G3D::Vector3 halfSize(size.x * 0.5f - bevel, size.y * 0.5f - bevel, size.z * 0.5f - bevel);
		BrickMesh::makeShadowBox(halfSize, m_shadowVertices, m_shadowIndices);
	}
}

template void MeshFactory<PBBMesh, 4>::flushCache();

template PBBMesh* MeshFactory<PBBMesh, 4>::create(const G3D::Vector3&, RenderSurfaceTypes);

template PBBMesh* MeshFactory<PBBMesh, 4>::createDecal(const G3D::Vector3&, NormalId);

template PBBMesh* MeshFactory<PBBMesh, 4>::createTexture(const G3D::Vector3&, NormalId, const G3D::Vector2&);

} // namespace RBX::ViewNew
