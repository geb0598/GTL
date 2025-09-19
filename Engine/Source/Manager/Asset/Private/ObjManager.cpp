#include "pch.h"
#include "Manager/Asset/Public/ObjManager.h"
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
	auto NewMesh = std::make_unique<FStaticMesh>();
	NewMesh->PathFileName = PathFileName;

	// 3. 큐브 데이터 하드코딩
	//    (정확한 조명을 위해 8개가 아닌 24개의 정점을 사용하여 각 면의 노멀을 다르게 지정)
	// Vertices
	NewMesh->Vertices = {
		// Front Face (+Z)
		{ FVector(-0.5f, -0.5f,  0.5f), FVector(0.f, 0.f, 1.f), FVector4(1,1,1,1), FVector2(0, 1) },
		{ FVector(-0.5f,  0.5f,  0.5f), FVector(0.f, 0.f, 1.f), FVector4(1,1,1,1), FVector2(0, 0) },
		{ FVector(0.5f,  0.5f,  0.5f), FVector(0.f, 0.f, 1.f), FVector4(1,1,1,1), FVector2(1, 0) },
		{ FVector(0.5f, -0.5f,  0.5f), FVector(0.f, 0.f, 1.f), FVector4(1,1,1,1), FVector2(1, 1) },
		// Back Face (-Z)
		{ FVector(0.5f, -0.5f, -0.5f), FVector(0.f, 0.f, -1.f), FVector4(1,1,1,1), FVector2(0, 1) },
		{ FVector(0.5f,  0.5f, -0.5f), FVector(0.f, 0.f, -1.f), FVector4(1,1,1,1), FVector2(0, 0) },
		{ FVector(-0.5f,  0.5f, -0.5f), FVector(0.f, 0.f, -1.f), FVector4(1,1,1,1), FVector2(1, 0) },
		{ FVector(-0.5f, -0.5f, -0.5f), FVector(0.f, 0.f, -1.f), FVector4(1,1,1,1), FVector2(1, 1) },
		// Left Face (-X)
		{ FVector(-0.5f, -0.5f, -0.5f), FVector(-1.f, 0.f, 0.f), FVector4(1,1,1,1), FVector2(0, 1) },
		{ FVector(-0.5f,  0.5f, -0.5f), FVector(-1.f, 0.f, 0.f), FVector4(1,1,1,1), FVector2(0, 0) },
		{ FVector(-0.5f,  0.5f,  0.5f), FVector(-1.f, 0.f, 0.f), FVector4(1,1,1,1), FVector2(1, 0) },
		{ FVector(-0.5f, -0.5f,  0.5f), FVector(-1.f, 0.f, 0.f), FVector4(1,1,1,1), FVector2(1, 1) },
		// Right Face (+X)
		{ FVector(0.5f, -0.5f,  0.5f), FVector(1.f, 0.f, 0.f), FVector4(1,1,1,1), FVector2(0, 1) },
		{ FVector(0.5f,  0.5f,  0.5f), FVector(1.f, 0.f, 0.f), FVector4(1,1,1,1), FVector2(0, 0) },
		{ FVector(0.5f,  0.5f, -0.5f), FVector(1.f, 0.f, 0.f), FVector4(1,1,1,1), FVector2(1, 0) },
		{ FVector(0.5f, -0.5f, -0.5f), FVector(1.f, 0.f, 0.f), FVector4(1,1,1,1), FVector2(1, 1) },
		// Top Face (+Y)
		{ FVector(-0.5f,  0.5f,  0.5f), FVector(0.f, 1.f, 0.f), FVector4(1,1,1,1), FVector2(0, 1) },
		{ FVector(-0.5f,  0.5f, -0.5f), FVector(0.f, 1.f, 0.f), FVector4(1,1,1,1), FVector2(0, 0) },
		{ FVector(0.5f,  0.5f, -0.5f), FVector(0.f, 1.f, 0.f), FVector4(1,1,1,1), FVector2(1, 0) },
		{ FVector(0.5f,  0.5f,  0.5f), FVector(0.f, 1.f, 0.f), FVector4(1,1,1,1), FVector2(1, 1) },
		// Bottom Face (-Y)
		{ FVector(-0.5f, -0.5f, -0.5f), FVector(0.f, -1.f, 0.f), FVector4(1,1,1,1), FVector2(0, 1) },
		{ FVector(-0.5f, -0.5f,  0.5f), FVector(0.f, -1.f, 0.f), FVector4(1,1,1,1), FVector2(0, 0) },
		{ FVector(0.5f, -0.5f,  0.5f), FVector(0.f, -1.f, 0.f), FVector4(1,1,1,1), FVector2(1, 0) },
		{ FVector(0.5f, -0.5f, -0.5f), FVector(0.f, -1.f, 0.f), FVector4(1,1,1,1), FVector2(1, 1) }
	};
	// Indices
	NewMesh->Indices = {
		// Front Face
		0, 1, 2,  0, 2, 3,
		// Back Face
		4, 5, 6,  4, 6, 7,
		// Left Face
		8, 9, 10, 8, 10, 11,
		// Right Face
		12, 13, 14, 12, 14, 15,
		// Top Face
		16, 17, 18, 16, 18, 19,
		// Bottom Face
		20, 21, 22, 20, 22, 23
	};

	// 4. emplace()를 사용하여 TMap에 소유권을 이전하고, 원시 포인터를 반환
	FStaticMesh* ReturnPtr = NewMesh.get();
	ObjFStaticMeshMap.emplace(PathFileName, std::move(NewMesh));

	return ReturnPtr;
}

UStaticMesh* FObjManager::LoadObjStaticMesh(const FString& PathFileName)
{
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
