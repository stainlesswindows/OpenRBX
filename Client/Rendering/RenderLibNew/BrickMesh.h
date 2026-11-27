#ifndef RENDERLIBNEW_BRICKMESH_H
#define RENDERLIBNEW_BRICKMESH_H

#include <G3D/Vector3.h>
#include <vector>

namespace RBX::ViewNew {

class BrickMesh
{
public:
	static void makeShadowBox(
		const G3D::Vector3& halfSize, std::vector<G3D::Vector3>& vertices, std::vector<unsigned short>& indices
	);

	static const float widthInset;
	static const float normalPerturbation;
};

extern bool brickMeshBevels;

} // namespace RBX::ViewNew

#endif // RENDERLIBNEW_BRICKMESH_H
