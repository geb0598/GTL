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

	StaticMesh = FObjManager::LoadObjStaticMesh("Data/fruits/fruits.obj");
	// Material = FObjManager::LoadObjMaterial("");
	Type = EPrimitiveType::StaticMesh;

	Vertices = &(StaticMesh.Get()->GetVertices());
	VertexBuffer = AssetManager.CreateVertexBuffer(*Vertices);
	NumVertices = Vertices->size();

	Indices = &(StaticMesh.Get()->GetIndices());
	IndexBuffer = AssetManager.CreateIndexBuffer(*Indices);
	NumIndices = Indices->size();

	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;
	//BoundingBox = &ResourceManager.GetAABB();
}

UStaticMeshComponent::~UStaticMeshComponent()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
	}
	if (IndexBuffer)
	{
		IndexBuffer->Release();
	}
}
