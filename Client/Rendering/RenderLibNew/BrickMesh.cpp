#include "BrickMesh.h"

namespace RBX::ViewNew {

bool brickMeshBevels = true;

const float BrickMesh::widthInset = 0.0f;

const float BrickMesh::normalPerturbation = 0.05f;

static void appendTriangles(std::vector<unsigned short>& indices, int v0, int v1, int v2, int v3)
{
	indices.push_back(v0);
	indices.push_back(v1);
	indices.push_back(v2);
	indices.push_back(v0);
	indices.push_back(v2);
	indices.push_back(v3);
}

void BrickMesh::makeShadowBox(
	const G3D::Vector3& halfSize, std::vector<G3D::Vector3>& vertices, std::vector<unsigned short>& indices
)
{
	vertices.reserve(8);
	indices.reserve(36);

	vertices.push_back(G3D::Vector3(-halfSize.x, -halfSize.y, -halfSize.z));
	vertices.push_back(G3D::Vector3(-halfSize.x, -halfSize.y, halfSize.z));
	vertices.push_back(G3D::Vector3(-halfSize.x, halfSize.y, -halfSize.z));
	vertices.push_back(G3D::Vector3(-halfSize.x, halfSize.y, halfSize.z));
	vertices.push_back(G3D::Vector3(halfSize.x, -halfSize.y, -halfSize.z));
	vertices.push_back(G3D::Vector3(halfSize.x, -halfSize.y, halfSize.z));
	vertices.push_back(G3D::Vector3(halfSize.x, halfSize.y, -halfSize.z));
	vertices.push_back(G3D::Vector3(halfSize.x, halfSize.y, halfSize.z));

	appendTriangles(indices, 7, 5, 4, 6);
	appendTriangles(indices, 2, 0, 1, 3);
	appendTriangles(indices, 7, 6, 2, 3);
	appendTriangles(indices, 1, 0, 4, 5);
	appendTriangles(indices, 3, 1, 5, 7);
	appendTriangles(indices, 6, 4, 0, 2);
}

} // namespace RBX::ViewNew
