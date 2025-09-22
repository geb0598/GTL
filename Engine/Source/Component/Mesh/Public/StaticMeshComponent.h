#pragma once
#include "Core/Public/Class.h"       // UObject 기반 클래스 및 매크로
#include "Core/Public/ObjectPtr.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"

UCLASS()
class UStaticMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

public:
	UStaticMeshComponent();
	~UStaticMeshComponent();

	//void Serialize(bool bIsLoading, Json Handle)
	//{
	//	Super::Serialize(IsLoading, Handle);

	//	if (bIsLoading)
	//	{
	//		FString assetName;
	//		Handle << "ObjStaticMeshAsset" << assetName;
	//		StaticMesh = FObjManager::LoadObjStaticMesh(assetName);
	//	}
	//	else
	//	{
	//		FString assetName = StaticMesh->GetAssetPathFileName();
	//		Handle << "ObjStaticMeshAsset" << assetName;
	//	}
	//}

public:
	UStaticMesh* GetStaticMesh() { return StaticMesh; }
	void SetStaticMesh(const FName& InObjPath);

	TObjectPtr<UClass> GetSpecificWidgetClass() const override;

private:
	TObjectPtr<UStaticMesh> StaticMesh;

	// MaterialList

};
