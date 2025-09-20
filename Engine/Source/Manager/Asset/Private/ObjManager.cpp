#include "pch.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Asset/Public/ObjImporter.h"
// ... 기타 필요한 include ...

// static 멤버 변수의 실체를 정의(메모리 할당)합니다.
TMap<FString, std::unique_ptr<FStaticMesh>> FObjManager::ObjFStaticMeshMap;
TMap<FString, std::unique_ptr<UStaticMesh>> FObjManager::ObjUStaticMeshMap;

FStaticMesh* FObjManager::LoadObjStaticMeshAsset(const FString& PathFileName)
{
	// 1. find()를 사용하여 캐시에 있는지 확인
	auto Iter = ObjFStaticMeshMap.find(PathFileName);
	if (Iter != ObjFStaticMeshMap.end())
	{
		// 찾았다면, iterator를 통해 값(unique_ptr)에 접근하고 .get()으로 원시 포인터 반환
		return Iter->second.get();
	}

	// 2. std::make_unique로 FStaticMesh 객체를 안전하게 생성

	FObjInfo ObjInfo = {};
	if (!FObjImporter::LoadObj(PathFileName, &ObjInfo))
	{
		UE_LOG_ERROR("파일 정보를 읽어오는데 실패했습니다: %s", PathFileName.c_str());
		return nullptr;
	}

	auto NewMesh = std::make_unique<FStaticMesh>();
	NewMesh->PathFileName = PathFileName;

	if (ObjInfo.ObjectInfoList.size() == 0)
	{
		UE_LOG_ERROR("오브젝트 정보를 찾을 수 없습니다");
		return nullptr;
	}

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
				assert("Texture coordinate index out of range" && NormalIndex < ObjInfo.NormalList.size());
				Vertex.TexCoord = ObjInfo.TexCoordList[TexCoordIndex];
			}

			size_t Index = NewMesh->Vertices.size();
			NewMesh->Vertices.push_back(Vertex);
			NewMesh->Indices.push_back(Index);
			VertexMap[Key] = Index;
		}
		else
		{
			NewMesh->Indices.push_back(It->second);
		}
	}

	// 4. emplace()를 사용하여 TMap에 소유권을 이전하고, 원시 포인터를 반환
	FStaticMesh* ReturnPtr = NewMesh.get();
	ObjFStaticMeshMap.emplace(PathFileName, std::move(NewMesh));

	return ReturnPtr;
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
