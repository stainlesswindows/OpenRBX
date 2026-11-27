#ifndef RENDERLIBNEW_BEVELMESH_H
#define RENDERLIBNEW_BEVELMESH_H

#include "QuadVolume.h"
#include "renderlibnew/Mesh.h"

#include <array>

namespace RBX::ViewNew {

class BevelMesh : public RenderNew::Mesh
{
private:
	static bool smoothShading;

public:
	struct CornerVertex
	{
		bool valid = false;
		G3D::Vector3 position;
		G3D::Vector3 normal;
		G3D::Vector2 tex;
	};

	using CornerVertices = std::array<std::array<CornerVertex, 6>, 8>;

	static bool setSmoothShadingEnabled(bool value);

	static void recordCorner(
		CornerVertices& corners,
		NormalId face,
		const G3D::Vector3& sourceVertex,
		const G3D::Vector3& localVertex,
		const G3D::Vector3& localNormal,
		const G3D::Vector2& tex,
		float textureScale = 0.5f
	);

	static void appendBevelGeometry(
		RenderNew::Mesh::Level& level,
		const CornerVertices& corners,
		const G3D::Vector2& solidTex = G3D::Vector2(0.125f, 0.25f)
	);

	class Builder : public LevelBuilder
	{
	protected:
		const float bevel;
		CornerVertices corners{};

		Builder(
			G3D::ReferenceCountedPointer<RenderNew::Mesh::Level>& level,
			G3D::Vector3 size,
			RenderSurfaceTypes surfaceTypes,
			float bevel
		)
			: LevelBuilder(level, size, surfaceTypes), bevel(bevel)
		{
		}

		void onFaceVertex(
			NormalId face,
			Purpose purpose,
			const G3D::Vector3& sourceVertex,
			const G3D::Vector3& vertex,
			const G3D::Vector3& normal,
			const G3D::Vector2& tex,
			const G3D::Vector3& radius
		) override;
		bool shouldCaptureFaceVertices(Purpose purpose) const override { return bevel > 0.0f && purpose != Decal; }

	public:
		void build(Purpose purpose) override;

		void operator()(
			G3D::Vector3& vertex,
			G3D::Vector3& normal,
			G3D::Vector2& tex,
			const G3D::Vector3& radius,
			const G3D::Vector2int16& ivertex
		)
		{
			if (vertex.x > 0) {
				vertex.x -= bevel;
			}
			else {
				vertex.x += bevel;
			}
			if (vertex.y > 0) {
				vertex.y -= bevel;
			}
			else {
				vertex.y += bevel;
			}
		}
	};
};

} // namespace RBX::ViewNew

#endif // RENDERLIBNEW_BEVELMESH_H
