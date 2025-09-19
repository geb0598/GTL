#include "pch.h"
#include "StaticMesh.h"

UMaterial* UStaticMesh::GetMaterial(int32 MaterialIndex) const
{
	return (MaterialIndex >= 0 && MaterialIndex < Materials.size()) ? Materials[MaterialIndex] : nullptr;
}

void UStaticMesh::SetMaterial(int32 MaterialIndex, UMaterial* Material)
{
	if (MaterialIndex >= 0 && MaterialIndex < Materials.size())
	{
		Materials[MaterialIndex] = Material;
	}
}

int32 UStaticMesh::GetNumMaterials() const
{
	return Materials.size();
}
