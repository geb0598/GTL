#pragma once
#include <Global/CoreTypes.h>
#include <Global/Types.h>
#include <Component/Mesh/Public/StaticMesh.h>
#include <Manager/Asset/Public/ObjImporter.h>
#include <memory>

class FObjManager
{
public:
	static FStaticMesh* LoadObjStaticMeshAsset(const FString& PathFileName, const FObjImporter::Configuration& Config = {});
	static UStaticMesh* LoadObjStaticMesh(const FString& PathFileName, const FObjImporter::Configuration& Config = {});
	static void CreateMaterialsFromMTL(UStaticMesh* StaticMesh, FStaticMesh* StaticMeshAsset, const FString& ObjFilePath);

	static constexpr size_t INVALID_INDEX = SIZE_MAX;

private:
	static TMap<FString, std::unique_ptr<FStaticMesh>> ObjFStaticMeshMap;
	static TMap<FString, std::unique_ptr<UStaticMesh>> ObjUStaticMeshMap; // 추후 UObject 관리 시스템이 생기면 이 맵은 제거하는 것을 권장
};
