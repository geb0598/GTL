#include "pch.h"
#include "Core/Public/Class.h"       // UObject 기반 클래스 및 매크로
#include "Core/Public/ObjectPtr.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UStaticMeshComponent, UMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
{
	UAssetManager& AssetManager = UAssetManager::GetInstance();

	FString DefaultObjPath = "Data/fruits/fruits.obj";

	StaticMesh = FObjManager::LoadObjStaticMesh(DefaultObjPath);
	// Material = FObjManager::LoadObjMaterial("");
	Type = EPrimitiveType::StaticMesh;

	Vertices = &(StaticMesh.Get()->GetVertices());
	VertexBuffer = AssetManager.GetVertexBuffer(DefaultObjPath);
	NumVertices = Vertices->size();

	Indices = &(StaticMesh.Get()->GetIndices());
	IndexBuffer = AssetManager.GetIndexBuffer(DefaultObjPath);
	NumIndices = Indices->size();

	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;
	//BoundingBox = &ResourceManager.GetAABB();
}

UStaticMeshComponent::~UStaticMeshComponent()
{
}
