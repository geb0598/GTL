#include "pch.h"
#include "Core/Public/Class.h"       // UObject 기반 클래스 및 매크로
#include "Core/Public/ObjectPtr.h"
#include "Core/Public/ObjectIterator.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"
#include "Render/UI/Widget/Public/StaticMeshComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"
#include "Texture/Public/Texture.h"

#include <algorithm>
#include <json.hpp>

#include "Editor/Public/Viewport.h"
#include "Render/Renderer/Public/Renderer.h"

IMPLEMENT_CLASS(UStaticMeshComponent, UMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
	: bIsScrollEnabled(false)
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

		JSON OverrideMaterialJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "OverrideMaterial", OverrideMaterialJson, nullptr, false))
		{
			for (auto& Pair : OverrideMaterialJson.ObjectRange())
			{
				const FString& IdString = Pair.first;
				JSON& MaterialPathDataJson = Pair.second;

				int32 MaterialId;
				try { MaterialId = std::stoi(IdString); }
				catch (const std::exception&) { continue; }

				FString MaterialPath;
				FJsonSerializer::ReadString(MaterialPathDataJson, "Path", MaterialPath);

				for (TObjectIterator<UMaterial> It; It; ++It)
				{
					UMaterial* Mat = *It;
					if (!Mat) continue;

					if (Mat->GetDiffuseTexture()->GetFilePath() == MaterialPath)
					{
						SetMaterial(MaterialId, Mat);
						break;
					}
				}
			}
		}
	}
	// 저장
	else
	{
		if (StaticMesh)
		{
			InOutHandle["ObjStaticMeshAsset"] = StaticMesh->GetAssetPathFileName().ToString();

			if (0 < OverrideMaterials.size())
			{
				int Idx = 0;
				JSON MaterialsJson = json::Object();
				for (const UMaterial* Material : OverrideMaterials)
				{
					JSON MaterialJson;
					MaterialJson["Path"] = Material->GetDiffuseTexture()->GetFilePath().ToString();

					MaterialsJson[std::to_string(Idx++)] = MaterialJson;
				}
				InOutHandle["OverrideMaterial"] = MaterialsJson;
			}
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

		// 원본 메시 경로 저장 (LOD 파일이 아닌 경우에만)
		FString PathStr = InObjPath.ToString();
		if (PathStr.find("_lod_") == FString::npos)
		{
			OriginalMeshPath = InObjPath;
		}

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

UMaterial* UStaticMeshComponent::GetMaterial(int32 Index) const
{
	if (Index >= 0 && Index < OverrideMaterials.size() && OverrideMaterials[Index])
	{
		return OverrideMaterials[Index];
	}
	return StaticMesh ? StaticMesh->GetMaterial(Index) : nullptr;
}

void UStaticMeshComponent::SetMaterial(int32 Index, UMaterial* InMaterial)
{
	if (Index < 0) return;

	if (Index >= OverrideMaterials.size())
	{
		OverrideMaterials.resize(Index + 1, nullptr);
	}
	OverrideMaterials[Index] = InMaterial;
}

// LOD System Implementation
void UStaticMeshComponent::SetLODLevel(int32 LODLevel)
{
	if (!bLODEnabled || !StaticMesh)
	{
		// UE_LOG("SetLODLevel: Early return - LODEnabled=%s, StaticMesh=%s",
			// bLODEnabled ? "true" : "false", StaticMesh ? "valid" : "nullptr");
		return;
	}

	// LOD 레벨 유효성 검사
	LODLevel = std::max(LODLevel, 0);

	// StaticMesh에서 사용 가능한 LOD 개수 확인
	int32 MaxLODs = StaticMesh->HasLODs() ? static_cast<int32>(StaticMesh->GetNumLODs()) : 0;

	// LOD 메시가 없는 경우 원본만 사용하도록 제한
	if (!StaticMesh->HasLODs() && LODLevel > 0)
	{
		LODLevel = 0;
	}
	else if (LODLevel > MaxLODs)
	{
		LODLevel = MaxLODs; // 최대 사용 가능한 LOD로 제한
	}

	// UE_LOG("SetLODLevel: Setting LOD level to %d", LODLevel);
	CurrentLODLevel = LODLevel;

	// 렌더링 데이터만 업데이트 (SetStaticMesh 호출하지 않음)
	UAssetManager& AssetManager = UAssetManager::GetInstance();

	if (LODLevel == 0)
	{
		// 원본 메시로 완전히 다시 로드 (SetStaticMesh와 동일한 방식)
		// UE_LOG("SetLODLevel: Using original mesh: %s", OriginalMeshPath.ToString().data());

		UStaticMesh* NewStaticMesh = FObjManager::LoadObjStaticMesh(OriginalMeshPath);
		if (NewStaticMesh)
		{
			StaticMesh = NewStaticMesh;

			Vertices = &(StaticMesh.Get()->GetVertices());
			VertexBuffer = AssetManager.GetVertexBuffer(OriginalMeshPath);
			NumVertices = static_cast<uint32>(Vertices->size());

			Indices = &(StaticMesh.Get()->GetIndices());
			IndexBuffer = AssetManager.GetIndexBuffer(OriginalMeshPath);
			NumIndices = static_cast<uint32>(Indices->size());

			RenderState.CullMode = ECullMode::Back;
			RenderState.FillMode = EFillMode::Solid;
			BoundingBox = &AssetManager.GetStaticMeshAABB(OriginalMeshPath);

			// UE_LOG("SetLODLevel: Restored original - Vertices=%d, Indices=%d, VB=%s, IB=%s",
				// NumVertices, NumIndices, VertexBuffer ? "valid" : "null", IndexBuffer ? "valid" : "null");
		}
	}
	else if (StaticMesh->HasLODs() && LODLevel <= static_cast<int32>(StaticMesh->GetNumLODs()))
	{
		// LOD 경로 구성 (원본 경로에서 LOD 경로 생성)
		FString OriginalPathStr = OriginalMeshPath.ToString();

		// 원본 경로에서 LOD 경로 생성 (예: Data/smokegrenade/smokegrenade.obj -> Data/smokegrenade/LOD/smokegrenade_lod_050.obj)
		std::filesystem::path OriginalPath(OriginalPathStr);
		FString OriginalStem = OriginalPath.stem().string();
		std::filesystem::path LODDir = OriginalPath.parent_path() / "LOD";

		FString LODSuffix = (LODLevel == 1) ? "_lod_050" : "_lod_025";
		std::filesystem::path LODPath = LODDir / (OriginalStem + LODSuffix + ".obj");

		FString LODPathStr = LODPath.string();
		std::replace(LODPathStr.begin(), LODPathStr.end(), '\\', '/');
		FName LODPathName(LODPathStr);

		// UE_LOG("SetLODLevel: Using LOD mesh: %s", LODPathName.ToString().data());

		// LOD 메시로 완전히 새로 로드
		UStaticMesh* NewLODStaticMesh = FObjManager::LoadObjStaticMesh(LODPathName);
		if (NewLODStaticMesh)
		{
			StaticMesh = NewLODStaticMesh;

			Vertices = &(StaticMesh.Get()->GetVertices());
			VertexBuffer = AssetManager.GetVertexBuffer(LODPathName);
			NumVertices = static_cast<uint32>(Vertices->size());

			Indices = &(StaticMesh.Get()->GetIndices());
			IndexBuffer = AssetManager.GetIndexBuffer(LODPathName);
			NumIndices = static_cast<uint32>(Indices->size());

			RenderState.CullMode = ECullMode::Back;
			RenderState.FillMode = EFillMode::Solid;
			BoundingBox = &AssetManager.GetStaticMeshAABB(LODPathName);

			// UE_LOG("SetLODLevel: LOD mesh updated - Vertices=%d, Indices=%d, VB=%s, IB=%s",
				// NumVertices, NumIndices, VertexBuffer ? "valid" : "null", IndexBuffer ? "valid" : "null");
		}
	}
}

void UStaticMeshComponent::UpdateLODBasedOnDistance(const FVector& CameraPosition)
{
	if (!bLODEnabled || !StaticMesh)
		return;

	FVector ComponentPosition = GetRelativeLocation();

	// 컴포넌트 위치와 카메라 위치 간의 제곱거리 계산 (sqrt 연산 제거로 성능 향상)
	FVector DeltaVector = CameraPosition - ComponentPosition;
	float DistanceSquared = DeltaVector.X * DeltaVector.X + DeltaVector.Y * DeltaVector.Y + DeltaVector.Z * DeltaVector.Z;

	// 제곱거리에 따른 LOD 레벨 결정
	int32 NewLODLevel = 0;

	if (DistanceSquared > LODDistanceSquared2)
	{
		NewLODLevel = 2; // 가장 멀리 있을 때 LOD 2 (25%)
	}
	else if (DistanceSquared > LODDistanceSquared1)
	{
		NewLODLevel = 1; // 중간 거리일 때 LOD 1 (50%)
	}
	else
	{
		NewLODLevel = 0; // 가까이 있을 때 원본
	}

	// 최소 LOD 레벨 제한 적용
	if (NewLODLevel < MinLODLevel)
	{
		NewLODLevel = MinLODLevel;
	}

	// LOD 레벨이 변경된 경우에만 업데이트
	if (NewLODLevel != CurrentLODLevel)
	{
		SetLODLevel(NewLODLevel);
	}
}

void UStaticMeshComponent::TickComponent()
{
	// 부모 클래스의 TickComponent 호출
	Super::TickComponent();

	// LOD가 활성화되어 있고 StaticMesh가 존재할 때만 LOD 업데이트
	if (bLODEnabled && StaticMesh)
	{
		// 모든 객체가 유효한지 확인
		URenderer* RendererPtr = nullptr;
		try {
			RendererPtr = &URenderer::GetInstance();
		}
		catch (...) {
			return; // Renderer가 초기화되지 않은 경우
		}

		if (!RendererPtr) return;

		FViewport* Viewport = RendererPtr->GetViewportClient();
		if (!Viewport) return;

		UCamera* Camera = Viewport->GetActiveCamera();
		if (!Camera) return;

		// 카메라 위치가 유효한지 확인하고 LOD 업데이트
		FVector CameraPosition = Camera->GetLocation();
		UpdateLODBasedOnDistance(CameraPosition);
	}
}
