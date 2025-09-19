#include "pch.h"
#include "Core/Public/Class.h"       // UObject 기반 클래스 및 매크로
#include "Core/Public/ObjectPtr.h"
#include "Component/Mesh/Public/StaticMeshComp.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Manager/Asset/Public/ObjManager.h"

IMPLEMENT_CLASS(UStaticMeshComp, UMeshComponent)

UStaticMeshComp::UStaticMeshComp()
{
	StaticMesh = FObjManager::LoadObjStaticMesh("");
	Type = EPrimitiveType::StaticMesh;

	//Vertices = &(StaticMesh.Get()->GetVertices());
	VertexBuffer = StaticMesh.Get()->GetVertexBuffer();
	NumVertices = Vertices->size();

	Indices = &(StaticMesh.Get()->GetIndices());
	IndexBuffer = StaticMesh.Get()->GetIndexBuffer();
	NumIndices = Indices->size();

	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;
	//BoundingBox = &ResourceManager.GetAABB();
}
