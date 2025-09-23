#include "pch.h"
#include "Core/Public/Class.h"       // UObject 기반 클래스 및 매크로
#include "Core/Public/ObjectPtr.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"
#include "Render/UI/Widget/Public/StaticMeshComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"

#include <json.hpp>

IMPLEMENT_CLASS(UStaticMeshComponent, UMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
{
	Type = EPrimitiveType::StaticMesh;

	FName DefaultObjPath = "Data/Cube/Cube.obj";
	SetStaticMesh(DefaultObjPath);
}

UStaticMeshComponent::~UStaticMeshComponent()
{
}

void UStaticMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		FString AssetPath;
		FJsonSerializer::ReadString(InOutHandle, "ObjStaticMeshAsset", AssetPath);
		SetStaticMesh(AssetPath);
	}
	// 저장
	else
	{
		if (StaticMesh)
		{
			InOutHandle["ObjStaticMeshAsset"] = StaticMesh->GetAssetPathFileName().ToString();
		}
	}
}

TObjectPtr<UClass> UStaticMeshComponent::GetSpecificWidgetClass() const
{
	return UStaticMeshComponentWidget::StaticClass();
}

void UStaticMeshComponent::SetStaticMesh(const FName& InObjPath)
{
	UAssetManager& AssetManager = UAssetManager::GetInstance();

	UStaticMesh* NewStaticMesh = FObjManager::LoadObjStaticMesh(InObjPath);

	if (NewStaticMesh)
	{
		StaticMesh = NewStaticMesh;

		Vertices = &(StaticMesh.Get()->GetVertices());
		VertexBuffer = AssetManager.GetVertexBuffer(InObjPath);
		NumVertices = Vertices->size();

		Indices = &(StaticMesh.Get()->GetIndices());
		IndexBuffer = AssetManager.GetIndexBuffer(InObjPath);
		NumIndices = Indices->size();

		RenderState.CullMode = ECullMode::Back;
		RenderState.FillMode = EFillMode::Solid;
		BoundingBox = &AssetManager.GetStaticMeshAABB(InObjPath);
	}
}
