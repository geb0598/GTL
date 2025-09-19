#pragma once
#include "Component/Public/MeshComponent.h"

class UStaticMesh;

UCLASS()
class UStaticMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

public:
	UStaticMesh* GetStaticMesh() { return StaticMesh; }

private:
	UStaticMesh* StaticMesh;
};
