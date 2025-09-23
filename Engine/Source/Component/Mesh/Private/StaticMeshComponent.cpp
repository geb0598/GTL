#include "pch.h"
#include "Core/Public/Class.h"       // UObject 기반 클래스 및 매크로
#include "Core/Public/ObjectPtr.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"
#include "Render/UI/Widget/Public/StaticMeshComponentWidget.h"

IMPLEMENT_CLASS(UStaticMeshComponent, UMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
{
	FName DefaultObjPath = "Data/Cube/Cube.obj";
	SetStaticMesh(DefaultObjPath);
}

void UStaticMeshComponent::SetStaticMesh(const FName& InObjPath)
{
	UAssetManager& AssetManager = UAssetManager::GetInstance();

	StaticMesh = FObjManager::LoadObjStaticMesh(InObjPath);

	// Material = FObjManager::LoadObjMaterial("");
	Type = EPrimitiveType::StaticMesh;

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

UStaticMeshComponent::~UStaticMeshComponent()
{
}

TObjectPtr<UClass> UStaticMeshComponent::GetSpecificWidgetClass() const
{
	return UStaticMeshComponentWidget::StaticClass();
}
