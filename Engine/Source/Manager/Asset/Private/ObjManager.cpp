#include "pch.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Asset/Public/ObjImporter.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include <filesystem>

// static 멤버 변수의 실체를 정의(메모리 할당)합니다.
TMap<FString, std::unique_ptr<FStaticMesh>> FObjManager::ObjFStaticMeshMap;
TMap<FString, std::unique_ptr<UStaticMesh>> FObjManager::ObjUStaticMeshMap;

/** @brief: Vertex Key for creating index buffer */
using VertexKey = std::tuple<size_t, size_t, size_t>;

struct VertexKeyHash
{
	std::size_t operator() (VertexKey Key) const
	{
		auto Hash1 = std::hash<size_t>{}(std::get<0>(Key));
		auto Hash2 = std::hash<size_t>{}(std::get<1>(Key));
		auto Hash3 = std::hash<size_t>{}(std::get<2>(Key));

		std::size_t Seed = Hash1;
		Seed ^= Hash2 + 0x9e3779b97f4a7c15ULL + (Seed << 6) + (Seed >> 2);
		Seed ^= Hash3 + 0x9e3779b97f4a7c15ULL + (Seed << 6) + (Seed >> 2);

		return Seed;
	}
};

/** @todo: std::filesystem으로 변경 */
FStaticMesh* FObjManager::LoadObjStaticMeshAsset(const FString& PathFileName, const FObjImporter::Configuration& Config)
{
	auto Iter = ObjFStaticMeshMap.find(PathFileName);
	if (Iter != ObjFStaticMeshMap.end())
	{
		return Iter->second.get();
	}

	/** #1. '.obj' 파일로부터 오브젝트 정보를 로드 */
	FObjInfo ObjInfo;
	if (!FObjImporter::LoadObj(PathFileName, &ObjInfo, Config))
	{
		UE_LOG_ERROR("파일 정보를 읽어오는데 실패했습니다: %s", PathFileName.c_str());
		return nullptr;
	}

	auto StaticMesh = std::make_unique<FStaticMesh>();
	StaticMesh->PathFileName = PathFileName;

	if (ObjInfo.ObjectInfoList.size() == 0)
	{
		UE_LOG_ERROR("오브젝트 정보를 찾을 수 없습니다");
		return nullptr;
	}

	/** #2. 오브젝트 정보로부터 버텍스 배열과 인덱스 배열을 구성 */
	/** @note: Use only first object in '.obj' file to create FStaticMesh. */
	FObjectInfo& ObjectInfo = ObjInfo.ObjectInfoList[0];

	TMap<VertexKey, size_t, VertexKeyHash> VertexMap;
	for (size_t i = 0; i < ObjectInfo.VertexIndexList.size(); ++i)
	{
		size_t VertexIndex = ObjectInfo.VertexIndexList[i];

		size_t NormalIndex = INVALID_INDEX;
		if (!ObjectInfo.NormalIndexList.empty())
		{
			NormalIndex = ObjectInfo.NormalIndexList[i];
		}

		size_t TexCoordIndex = INVALID_INDEX;
		if (!ObjectInfo.TexCoordIndexList.empty())
		{
			TexCoordIndex = ObjectInfo.TexCoordIndexList[i];
		}

		VertexKey Key{ VertexIndex, NormalIndex, TexCoordIndex };
		auto It = VertexMap.find(Key);
		if (It == VertexMap.end())
		{
			FNormalVertex Vertex = {};
			Vertex.Position = ObjInfo.VertexList[VertexIndex];

			if (NormalIndex != INVALID_INDEX)
			{
				assert("Vertex normal index out of range" && NormalIndex < ObjInfo.NormalList.size());
				Vertex.Normal = ObjInfo.NormalList[NormalIndex];
			}

			if (TexCoordIndex != INVALID_INDEX)
			{
				assert("Texture coordinate index out of range" && TexCoordIndex < ObjInfo.TexCoordList.size());
				Vertex.TexCoord = ObjInfo.TexCoordList[TexCoordIndex];
			}

			size_t Index = StaticMesh->Vertices.size();
			StaticMesh->Vertices.push_back(Vertex);
			StaticMesh->Indices.push_back(Index);
			VertexMap[Key] = Index;
		}
		else
		{
			StaticMesh->Indices.push_back(It->second);
		}
	}

	/** #3. 오브젝트가 사용하는 머티리얼의 목록을 저장 */
	StaticMesh->MaterialInfo.resize(ObjectInfo.MaterialNameList.size());
	for (size_t i = 0; i < ObjectInfo.MaterialNameList.size(); ++i)
	{
		for (size_t j = 0; j < ObjInfo.ObjectMaterialInfoList.size(); ++j)
		{
			if (ObjectInfo.MaterialNameList[i] == ObjInfo.ObjectMaterialInfoList[j].Name)
			{
				StaticMesh->MaterialInfo[i].Name			= std::move(ObjInfo.ObjectMaterialInfoList[j].Name);
				StaticMesh->MaterialInfo[i].Ka				= std::move(ObjInfo.ObjectMaterialInfoList[j].Ka);
				StaticMesh->MaterialInfo[i].Kd				= std::move(ObjInfo.ObjectMaterialInfoList[j].Kd);
				StaticMesh->MaterialInfo[i].Ks				= std::move(ObjInfo.ObjectMaterialInfoList[j].Ks);
				StaticMesh->MaterialInfo[i].Ke				= std::move(ObjInfo.ObjectMaterialInfoList[j].Ke);
				StaticMesh->MaterialInfo[i].Ns				= std::move(ObjInfo.ObjectMaterialInfoList[j].Ns);
				StaticMesh->MaterialInfo[i].Ni				= std::move(ObjInfo.ObjectMaterialInfoList[j].Ni);
				StaticMesh->MaterialInfo[i].D				= std::move(ObjInfo.ObjectMaterialInfoList[j].D);
				StaticMesh->MaterialInfo[i].Illumination	= std::move(ObjInfo.ObjectMaterialInfoList[j].Illumination);
				StaticMesh->MaterialInfo[i].KaMap			= std::move(ObjInfo.ObjectMaterialInfoList[j].KaMap);
				StaticMesh->MaterialInfo[i].KdMap			= std::move(ObjInfo.ObjectMaterialInfoList[j].KdMap);
				StaticMesh->MaterialInfo[i].KsMap			= std::move(ObjInfo.ObjectMaterialInfoList[j].KsMap);
				StaticMesh->MaterialInfo[i].NsMap			= std::move(ObjInfo.ObjectMaterialInfoList[j].NsMap);
				StaticMesh->MaterialInfo[i].DMap			= std::move(ObjInfo.ObjectMaterialInfoList[j].DMap);
				StaticMesh->MaterialInfo[i].BumpMap			= std::move(ObjInfo.ObjectMaterialInfoList[j].BumpMap);

				continue;
			}
		}
	}

	/** #4. 오브젝트의 서브메쉬 정보를 저장 */

	StaticMesh->Sections.resize(ObjectInfo.MaterialIndexList.size());
	for (size_t i = 0; i < ObjectInfo.MaterialIndexList.size(); ++i)
	{
		StaticMesh->Sections[i].StartIndex = ObjectInfo.MaterialIndexList[i] * 3;

		if (i < ObjectInfo.MaterialIndexList.size() - 1)
		{
			StaticMesh->Sections[i].IndexCount = (ObjectInfo.MaterialIndexList[i + 1] - ObjectInfo.MaterialIndexList[i]) * 3;
		}
		else
		{
			StaticMesh->Sections[i].IndexCount = (StaticMesh->Indices.size() / 3 - ObjectInfo.MaterialIndexList[i]) * 3;
		}

		StaticMesh->Sections[i].MaterialSlot = i;
	}

	ObjFStaticMeshMap.emplace(PathFileName, std::move(StaticMesh));

	return ObjFStaticMeshMap[PathFileName].get();
}

/**
 * @brief MTL 정보를 바탕으로 UStaticMesh에 재질을 설정하는 함수
 */
void FObjManager::CreateMaterialsFromMTL(UStaticMesh* StaticMesh, FStaticMesh* StaticMeshAsset, const FString& ObjFilePath)
{
	if (!StaticMesh || !StaticMeshAsset || StaticMeshAsset->MaterialInfo.empty())
	{
		return;
	}

	// OBJ 파일이 있는 디렉토리 경로 추출
	std::filesystem::path ObjPath(ObjFilePath);
	std::filesystem::path ObjDirectory = ObjPath.parent_path();

	UAssetManager& AssetManager = UAssetManager::GetInstance();

	// MaterialInfo 개수만큼 UMaterial 배열 크기 설정
	size_t MaterialCount = StaticMeshAsset->MaterialInfo.size();
	for (size_t i = 0; i < MaterialCount; ++i)
	{
		const FMaterial& MaterialInfo = StaticMeshAsset->MaterialInfo[i];
		
		// UMaterial 객체 생성
		auto* Material = new UMaterial();
		
		// Diffuse 텍스처 로드 (map_Kd)
		if (!MaterialInfo.KdMap.empty())
		{
			std::filesystem::path TexturePath = ObjDirectory / MaterialInfo.KdMap;
			FString TexturePathStr = TexturePath.string();
			
			UE_LOG("텍스처 로드 시도: %s -> %s", MaterialInfo.KdMap.c_str(), TexturePathStr.c_str());
			
			// 파일 존재 여부 확인
			if (std::filesystem::exists(TexturePath))
			{
				UE_LOG("텍스처 파일 존재: %s", TexturePathStr.c_str());
				UTexture* DiffuseTexture = AssetManager.CreateTexture(TexturePathStr);
				if (DiffuseTexture)
				{
					Material->SetDiffuseTexture(DiffuseTexture);
					UE_LOG("재질 %s에 Diffuse 텍스처 로드 성공: %s", MaterialInfo.Name.c_str(), TexturePathStr.c_str());
				}
				else
				{
					UE_LOG_ERROR("텍스처 생성 실패: %s", TexturePathStr.c_str());
				}
			}
			else
			{
				UE_LOG_ERROR("텍스처 파일 없음: %s", TexturePathStr.c_str());
			}
		}
		
		// Ambient 텍스처 로드 (map_Ka)
		if (!MaterialInfo.KaMap.empty())
		{
			std::filesystem::path TexturePath = ObjDirectory / MaterialInfo.KaMap;
			FString TexturePathStr = TexturePath.string();
			
			UTexture* AmbientTexture = AssetManager.CreateTexture(TexturePathStr);
			if (AmbientTexture)
			{
				Material->SetAmbientTexture(AmbientTexture);
			}
		}
		
		// Specular 텍스처 로드 (map_Ks)
		if (!MaterialInfo.KsMap.empty())
		{
			std::filesystem::path TexturePath = ObjDirectory / MaterialInfo.KsMap;
			FString TexturePathStr = TexturePath.string();
			
			UTexture* SpecularTexture = AssetManager.CreateTexture(TexturePathStr);
			if (SpecularTexture)
			{
				Material->SetSpecularTexture(SpecularTexture);
			}
		}
		
		// Alpha 텍스처 로드 (map_d)
		if (!MaterialInfo.DMap.empty())
		{
			std::filesystem::path TexturePath = ObjDirectory / MaterialInfo.DMap;
			FString TexturePathStr = TexturePath.string();
			
			UTexture* AlphaTexture = AssetManager.CreateTexture(TexturePathStr);
			if (AlphaTexture)
			{
				Material->SetAlphaTexture(AlphaTexture);
			}
		}
		
		// UStaticMesh에 재질 설정
		StaticMesh->SetMaterial(static_cast<int32>(i), Material);
	}
}

UStaticMesh* FObjManager::LoadObjStaticMesh(const FString& PathFileName, const FObjImporter::Configuration& Config)
{
	// Map에 해당 키가 이미 있는지 확인합니다.
	auto Iter = ObjUStaticMeshMap.find(PathFileName);
	if (Iter != ObjUStaticMeshMap.end())
	{
		// 이미 있다면, 저장된 unique_ptr에서 raw pointer를 얻어 반환합니다.
		return Iter->second.get();
	}

	//for (TObjectIterator<UStaticMesh> It; It; ++It)
	//{
	//	UStaticMesh* StaticMesh = *It;
	//	if (StaticMesh->GetAssetPathFileName() == PathFileName)
	//		return It;
	//}

	FStaticMesh* StaticMeshAsset = FObjManager::LoadObjStaticMeshAsset(PathFileName, Config);
	UStaticMesh* StaticMesh = new UStaticMesh();
	ObjUStaticMeshMap.emplace(PathFileName, std::move(StaticMesh));
	StaticMesh->SetStaticMeshAsset(StaticMeshAsset);

	// MTL 정보를 바탕으로 재질 객체 생성
	CreateMaterialsFromMTL(StaticMesh, StaticMeshAsset, PathFileName);

	return StaticMesh;
}
