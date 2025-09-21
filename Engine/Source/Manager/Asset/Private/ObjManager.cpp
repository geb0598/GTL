#include "pch.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Asset/Public/ObjImporter.h"
// ... 기타 필요한 include ...

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
FStaticMesh* FObjManager::LoadObjStaticMeshAsset(const FString& PathFileName)
{
	auto Iter = ObjFStaticMeshMap.find(PathFileName);
	if (Iter != ObjFStaticMeshMap.end())
	{
		return Iter->second.get();
	}

	/** #1. '.obj' 파일로부터 오브젝트 정보를 로드 */
	FObjInfo ObjInfo;
	if (!FObjImporter::LoadObj(PathFileName, &ObjInfo))
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

UStaticMesh* FObjManager::LoadObjStaticMesh(const FString& PathFileName)
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

	FStaticMesh* StaticMeshAsset = FObjManager::LoadObjStaticMeshAsset(PathFileName);
	UStaticMesh* StaticMesh = new UStaticMesh();
	ObjUStaticMeshMap.emplace(PathFileName, std::move(StaticMesh));
	StaticMesh->SetStaticMeshAsset(StaticMeshAsset);
	return StaticMesh;
}
