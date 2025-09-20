#pragma once
#include <Global/CoreTypes.h>
#include <Global/Types.h>
#include <Component/Mesh/Public/StaticMesh.h>
#include <memory>

class FObjManager
{
public:
	static FStaticMesh* LoadObjStaticMeshAsset(const FString& PathFileName);
	static UStaticMesh* LoadObjStaticMesh(const FString& PathFileName);

private:
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

	static constexpr size_t INVALID_INDEX = SIZE_MAX;

	static TMap<FString, std::unique_ptr<FStaticMesh>> ObjFStaticMeshMap;
	static TMap<FString, std::unique_ptr<UStaticMesh>> ObjUStaticMeshMap; // 추후 UObject 관리 시스템이 생기면 이 맵은 제거하는 것을 권장
};
