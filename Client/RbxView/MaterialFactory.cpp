#include "MaterialFactory.h"

#include "util/ContentProvider.h"

#include <G3D/GImage.h>
#include <algorithm>

namespace RBX::View {

static float specular = 0.9f;
static float shiny = 50.0f;

G3D::ReferenceCountedPointer<Render::TextureProxy> MaterialFactory::getSurfacesTexture(G3D::Color3 tint)
{
	return surfacesTexture->shade(tint, Render::TextureProxyRef());
}

G3D::ReferenceCountedPointer<Render::Material> MaterialFactory::getMaterial(Attributes attributes)
{
	G3D::ReferenceCountedPointer<Render::Material> result;
	Database::iterator iter = database.find(attributes);
	if (iter != database.end()) {
		result = iter->second.createStrongPtr();
	}

	if (result.isNull()) {
		G3D::Color3 color3 = attributes.color.color3();
		G3D::ReferenceCountedPointer<Render::TextureProxy> texture = getSurfacesTexture(color3);

		result = new Render::Material();
		if (attributes.transparency == 1.0f) {
			result->appendEmptyLevel();
		}
		else {
			result->appendLevel(
				texture, G3D::Color3::WHITE, specular, shiny, attributes.reflectance, attributes.transparency
			);
		}
		database[attributes] = result;
	}

	return result;
}

MaterialFactory::MaterialFactory(G3D::TextureManager* textureManager)
{
	surfacesTexture = new Render::TextureProxy(
		*textureManager, ContentProvider::singleton().getAssetFile("textures\\surfaces.png"), true
	);
	fileMeshTexture = new Render::TextureProxy(
		*textureManager, ContentProvider::singleton().getAssetFile("textures\\JohnTex.png"), false
	);
}

} // namespace RBX::View
