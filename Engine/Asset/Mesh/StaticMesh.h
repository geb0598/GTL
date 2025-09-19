#pragma once
#include "Core/Public/Object.h"
#include "Global/Types.h"

struct FStaticMesh;
class UMaterial;

UCLASS()
class UStaticMesh : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMesh, UObject)

public:
	FStaticMesh* GetStaticMeshAsset() { return StaticMeshAsset; }
	
	UMaterial* GetMaterial(int32 MaterialIndex) const;
	void SetMaterial(int32 MaterialIndex, UMaterial* Material);
	int32 GetNumMaterials() const;

private:
	FStaticMesh* StaticMeshAsset;
	TArray<UMaterial*> Materials;
};
