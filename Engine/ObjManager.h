#pragma once

#include "Core/Public/Object.h"
#include "Source/Global/Types.h"

class FStaticMesh;
class UStaticMesh;

UCLASS()
class FObjManager
	: public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(FObjManager, UObject)
		
public:
	static FStaticMesh* LoadObjStaticMeshAsset(const FString& FileName)
	{
		// TODO
		//if (It = ObjStaticMeshMap.find(FileName))
		//{
		//	return It;
		//}
		//
		// Obj Parse
		// ObjStaticMeshMap.emplace(FileName, NewStaticMesh)
		// return NewStaticMesh
	}

	static UStaticMesh* LoadObjStaticMesh(const FString& FileName)
	{
		// TODO
		//for (TObjectIterator<UStaticMesh> It; It; ++It)
		//{
		//	UStaticMesh* StaticMesh = *It;
		//	if (StaticMesh->GetAssetPathFileName() == PathFileName)
		//	{
		//		return It;
		//	}
		//}

		//FStaticMesh* Asset = FObjManager::LoadObjStaticMesh(PathFileName);
		//UStaticMesh* StaticMesh = ConstructObject<UStaticMesh>();
		//StaticMesh->SetStaticMeshAsset(StaticMeshAsset);
	}
	
private:
	static TMap<FString, FStaticMesh*> ObjStaticMeshMap;
};
