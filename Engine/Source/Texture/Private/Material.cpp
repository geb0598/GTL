#include "pch.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/TextureRenderProxy.h"

IMPLEMENT_CLASS(UMaterial, UObject)

UMaterial::~UMaterial()
{
	if (DiffuseTexture) { delete DiffuseTexture; }
	if (AmbientTexture) { delete AmbientTexture; }
	if (SpecularTexture) { delete SpecularTexture; }
	if (NormalTexture) { delete NormalTexture; }
	if (AlphaTexture) { delete AlphaTexture; }
	if (BumpTexture) { delete BumpTexture; }
}
