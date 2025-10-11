#include "pch.h"
#include "Component/Public/DecalComponent.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h" 
#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"
#include <json.hpp>
#include <filesystem>

IMPLEMENT_CLASS(UDecalComponent, UPrimitiveComponent)

UDecalComponent::UDecalComponent() : DecalMaterial(nullptr)
{
	UAssetManager& AssetManager = UAssetManager::GetInstance();
	UAssetManager& ResourceManager = UAssetManager::GetInstance();

	DecalMaterial = AssetManager.CreateMaterial(FName("recovery_256x"), FName("Asset/Texture/recovery_256x.png"));

	Type = EPrimitiveType::Decal;
	Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);

	Indices = ResourceManager.GetIndexData(Type);
	IndexBuffer = ResourceManager.GetIndexbuffer(Type);
	NumIndices = ResourceManager.GetNumIndices(Type);

	BoundingBox = &ResourceManager.GetAABB(Type);

	RenderState.CullMode = ECullMode::None;
	RenderState.FillMode = EFillMode::Solid;
}

UDecalComponent::~UDecalComponent()
{
	DecalMaterial = nullptr;
}

void UDecalComponent::SetDecalMaterial(UMaterial* InMaterial)
{
	if (DecalMaterial == InMaterial) { return; }

	DecalMaterial = InMaterial;
}

UMaterial* UDecalComponent::GetDecalMaterial() const
{
	return DecalMaterial;
}

UObject* UDecalComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UDecalComponent*>(Super::Duplicate(Parameters));

	DupObject->DecalMaterial = DecalMaterial;

	return DupObject;
}

void UDecalComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// --- 불러오기 ---
	if (bInIsLoading)
	{
		FString MaterialName, TexturePath;
		FJsonSerializer::ReadString(InOutHandle, "DecalMaterialName", MaterialName, "", false);
		FJsonSerializer::ReadString(InOutHandle, "DecalMaterialTexturePath", TexturePath, "", false);

		if (!MaterialName.empty())
		{
			UAssetManager& AssetManager = UAssetManager::GetInstance();
			UMaterial* LoadedMaterial = AssetManager.CreateMaterial(FName(MaterialName), FName(TexturePath));
			SetDecalMaterial(LoadedMaterial);
		}
	}
	// --- 저장하기 ---
	else
	{
		if (DecalMaterial)
		{
			// 재질의 이름과 텍스처 경로를 가져옵니다.
			const FString& MaterialName = DecalMaterial->GetName().ToString();
			const FString& TexturePath = DecalMaterial->GetDiffuseTexture() ? DecalMaterial->GetDiffuseTexture()->GetFilePath().ToString() : "";
			InOutHandle["DecalMaterialName"] = MaterialName;
			InOutHandle["DecalMaterialTexturePath"] = TexturePath;
		}
	}
}
