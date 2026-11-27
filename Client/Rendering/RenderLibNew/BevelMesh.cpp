#include "BevelMesh.h"

#include <algorithm>
#include <cmath>

namespace RBX::ViewNew {
namespace {

NormalId faceForAxis(int axis, bool positive)
{
	return static_cast<NormalId>(axis + (positive ? 0 : 3));
}

int cornerIndex(const G3D::Vector3& vertex)
{
	return (vertex.x > 0.0f ? 1 : 0) | (vertex.y > 0.0f ? 2 : 0) | (vertex.z > 0.0f ? 4 : 0);
}

void appendFacet(
	RenderNew::Mesh::Level& level,
	const G3D::Vector3* positions,
	const G3D::Vector2* texCoords,
	int vertexCount,
	G3D::Vector3 normal,
	const G3D::Vector3* vertexNormals
)
{
	if (normal.magnitude() < 1e-4f) {
		return;
	}

	normal = normal.direction();
	unsigned int indices[4];
	for (int i = 0; i < vertexCount; ++i) {
		G3D::Vector3 vertexNormal = normal;
		if (vertexNormals != nullptr && vertexNormals[i].magnitude() > 1e-4f) {
			vertexNormal = vertexNormals[i].direction();
		}
		indices[i] = RenderNew::Mesh::allocVertex(positions[i], vertexNormal, texCoords[i], true);
	}

	if (level.primitive == G3D::RenderDevice::QUADS) {
		if (vertexCount == 4) {
			level.indexArray.append(indices[0], indices[1], indices[2], indices[3]);
		}
		else {
			level.indexArray.append(indices[0], indices[1], indices[2], indices[0]);
		}
	}
	else if (vertexCount == 4) {
		level.indexArray.append(indices[0], indices[1], indices[2]);
		level.indexArray.append(indices[0], indices[2], indices[3]);
	}
	else {
		level.indexArray.append(indices[0], indices[1], indices[2]);
	}
}

} // namespace

bool BevelMesh::smoothShading = false;

bool BevelMesh::setSmoothShadingEnabled(bool value)
{
	if (smoothShading == value) {
		return false;
	}

	smoothShading = value;
	return true;
}

void BevelMesh::recordCorner(
	CornerVertices& corners,
	NormalId face,
	const G3D::Vector3& sourceVertex,
	const G3D::Vector3& localVertex,
	const G3D::Vector3& localNormal,
	const G3D::Vector2& tex,
	float textureScale
)
{
	const int corner = cornerIndex(RBX::uvwToObject(sourceVertex, face));
	CornerVertex& result = corners[corner][face];
	result.valid = true;
	result.position = RBX::uvwToObject(localVertex, face);
	result.normal = RBX::uvwToObject(localNormal, face);
	result.tex = tex * textureScale;
}

void BevelMesh::appendBevelGeometry(
	RenderNew::Mesh::Level& level, const CornerVertices& corners, const G3D::Vector2& solidTex
)
{
	for (int corner = 0; corner < 8; ++corner) {
		for (int axis = 0; axis < 3; ++axis) {
			if (!corners[corner][faceForAxis(axis, (corner & (1 << axis)) != 0)].valid) {
				return;
			}
		}
	}

	for (int axis = 0; axis < 3; ++axis) {
		const int aAxis = (axis + 1) % 3;
		const int bAxis = (axis + 2) % 3;
		for (int signs = 0; signs < 4; ++signs) {
			const int corner0 = ((signs & 1) ? 1 << aAxis : 0) | ((signs & 2) ? 1 << bAxis : 0);
			const int corner1 = corner0 | (1 << axis);
			const NormalId faceA = faceForAxis(aAxis, (corner0 & (1 << aAxis)) != 0);
			const NormalId faceB = faceForAxis(bAxis, (corner0 & (1 << bAxis)) != 0);

			const CornerVertex& a0 = corners[corner0][faceA];
			const CornerVertex& b0 = corners[corner0][faceB];
			const CornerVertex& b1 = corners[corner1][faceB];
			const CornerVertex& a1 = corners[corner1][faceA];

			G3D::Vector3 positions[4] = {a0.position, b0.position, b1.position, a1.position};
			G3D::Vector3 normals[4] = {a0.normal, b0.normal, b1.normal, a1.normal};
			G3D::Vector2 texCoords[4] = {solidTex, solidTex, solidTex, solidTex};
			G3D::Vector3 expectedNormal = a0.normal + b0.normal + b1.normal + a1.normal;
			G3D::Vector3 normal = (positions[1] - positions[0]).cross(positions[3] - positions[0]);
			if (normal.dot(expectedNormal) < 0.0f) {
				std::swap(positions[1], positions[3]);
				std::swap(normals[1], normals[3]);
				std::swap(texCoords[1], texCoords[3]);
				normal = -normal;
			}
			appendFacet(level, positions, texCoords, 4, normal, smoothShading ? normals : nullptr);
		}
	}

	for (int corner = 0; corner < 8; ++corner) {
		const CornerVertex& x = corners[corner][faceForAxis(0, (corner & 1) != 0)];
		const CornerVertex& y = corners[corner][faceForAxis(1, (corner & 2) != 0)];
		const CornerVertex& z = corners[corner][faceForAxis(2, (corner & 4) != 0)];

		G3D::Vector3 positions[4] = {x.position, y.position, z.position, G3D::Vector3::zero()};
		G3D::Vector3 normals[4] = {x.normal, y.normal, z.normal, G3D::Vector3::zero()};
		G3D::Vector2 texCoords[4] = {solidTex, solidTex, solidTex, solidTex};
		G3D::Vector3 expectedNormal = x.normal + y.normal + z.normal;
		G3D::Vector3 normal = (positions[1] - positions[0]).cross(positions[2] - positions[0]);
		if (normal.dot(expectedNormal) < 0.0f) {
			std::swap(positions[1], positions[2]);
			std::swap(normals[1], normals[2]);
			std::swap(texCoords[1], texCoords[2]);
			normal = -normal;
		}
		appendFacet(level, positions, texCoords, 3, normal, smoothShading ? normals : nullptr);
	}
}

void BevelMesh::Builder::onFaceVertex(
	NormalId face,
	Purpose purpose,
	const G3D::Vector3& sourceVertex,
	const G3D::Vector3& vertex,
	const G3D::Vector3& normal,
	const G3D::Vector2& tex,
	const G3D::Vector3& radius
)
{
	if (bevel <= 0.0f || purpose == Decal || std::fabs(std::fabs(sourceVertex.x) - radius.x) > 1e-4f ||
		std::fabs(std::fabs(sourceVertex.y) - radius.y) > 1e-4f) {
		return;
	}
	recordCorner(corners, face, sourceVertex, vertex, normal, tex);
}

void BevelMesh::Builder::build(Purpose purpose)
{
	corners = CornerVertices{};
	LevelBuilder::build(purpose);
	if (bevel > 0.0f) {
		appendBevelGeometry(*level, corners);
	}
}

} // namespace RBX::ViewNew
