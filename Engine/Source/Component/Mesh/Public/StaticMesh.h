#pragma once

#include "Core/Public/Object.h"       // UObject 기반 클래스 및 매크로
#include "Core/Public/ObjectPtr.h" // TObjectPtr 사용
#include "Global/CoreTypes.h"        // TArray 등
#include "Physics/Public/AABB.h"

// 전방 선언: FStaticMesh의 전체 정의를 포함할 필요 없이 포인터만 사용
struct FMeshSection
{
	uint32 StartIndex;
	uint32 IndexCount;
	uint32 MaterialSlot;
};

struct FTriangleBVHPrimitive
{
	FAABB Bounds;
	FVector Center;
	uint32 Indices[3];

	// Precomputed for fast intersection in *model space*
	FVector V0;   // Vertices[Indices[0]].Position
	FVector E0;   // V1 - V0
	FVector E1;   // V2 - V0
};

struct FTriangleBVHNode
{
	FAABB Bounds;
	int32 LeftChild = -1;
	int32 RightChild = -1;
	int32 Start = 0;
	int32 Count = 0;
	bool bIsLeaf = false;
};

static constexpr float MT_EPS = 1e-8f;

FORCEINLINE bool RayHitTriangle_MT(
	const FRay& ray,           // model-space
	const FVector& V0,
	const FVector& E0,         // V1 - V0
	const FVector& E1,         // V2 - V0
	float tMax,                // current closest t (for pruning)
	float& outT)
{
	FVector rayOrigin = FVector(ray.Origin.X, ray.Origin.Y, ray.Origin.Z);
	FVector rayDirection = FVector(ray.Direction.X, ray.Direction.Y, ray.Direction.Z);

	// pvec = D x E1
	const FVector pvec = rayDirection.Cross(E1);
	const float det = E0.Dot(pvec);

	// Cull near-degenerate triangles, accept both sides (no backface cull).
	if (fabsf(det) < MT_EPS) return false;
	const float invDet = 1.0f / det;

	// tvec = O - V0
	const FVector tvec = rayOrigin - V0;

	// u = dot(tvec, pvec) * invDet
	const float u = (tvec.Dot(pvec)) * invDet;
	if (u < 0.0f || u > 1.0f) return false;

	// qvec = tvec x E0
	const FVector qvec = tvec.Cross(E0);

	// v = dot(D, qvec) * invDet
	const float v = (rayDirection.Dot(qvec)) * invDet;
	if (v < 0.0f || u + v > 1.0f) return false;

	// t = dot(E1, qvec) * invDet
	const float t = (E1.Dot(qvec)) * invDet;
	if (t <= 0.0f || t >= tMax) return false;

	outT = t;
	return true;
}


// Cooked Data
struct FStaticMesh
{
	FName PathFileName;

	TArray<FNormalVertex> Vertices;
	TArray<uint32> Indices;

	// --- 2. 재질 정보 (Materials) ---
	// 이 메시에 사용되는 모든 고유 재질의 목록 (페인트 팔레트)
	//TArray<UMaterial> Materials;

	TArray<FMaterial> MaterialInfo;

	// --- 3. 연결 정보 (Sections) ---
	// 각 재질을 어떤 기하 구간에 칠할지에 대한 지시서
	TArray<FMeshSection> Sections;

	// Triangle BVH cache (mutable so we can build lazily in const accessors)
	mutable bool bTriangleBVHDirty = true;
	mutable int32 TriangleBVHRoot = -1;
	mutable TArray<FTriangleBVHNode> TriangleBVHNodes;
	mutable TArray<FTriangleBVHPrimitive> TriangleBVHPrimitives;
};

/**
 * @brief FStaticMesh(Cooked Data)를 엔진 오브젝트 시스템에 통합하는 래퍼 클래스.
 * 가비지 컬렉션, 리플렉션, 애셋 참조 관리의 대상이 됩니다.
 */
UCLASS()
class UStaticMesh : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMesh, UObject)

public:
	UStaticMesh();
	virtual ~UStaticMesh(); // 가상 소멸자 권장

	// --- 데이터 연결 ---

	/**
	 * @brief AssetManager가 이 UStaticMesh 객체와 실제 데이터(FStaticMesh)를 연결합니다.
	 * @param InStaticMeshAsset AssetManager가 소유하고 있는 FStaticMesh 데이터에 대한 포인터
	 */
	FStaticMesh* GetStaticMeshAsset() { return StaticMeshAsset; }
	void SetStaticMeshAsset(FStaticMesh* InStaticMeshAsset);

	// --- 데이터 접근자 (Getters) ---
	// 이 UStaticMesh가 감싸고 있는 FStaticMesh의 데이터에 대한 접근을 제공합니다.
	// 렌더러나 컴포넌트는 이 함수들을 통해 실제 데이터에 접근합니다.

	const FName& GetAssetPathFileName() const;

	// Geometry Data
	const TArray<FNormalVertex>& GetVertices() const;
	TArray<FNormalVertex>& GetVertices();
	const TArray<uint32>& GetIndices() const;

	bool RaycastTriangleBVH(const FRay& ModelRay, float& InOutDistance) const;

	// LOD System
	void AddLODMesh(FStaticMesh* LODMesh);
	FStaticMesh* GetLODMesh(int32 LODLevel) const;
	int32 GetNumLODs() const { return LODMeshes.size(); }
	bool HasLODs() const { return !LODMeshes.empty(); }

	// Material Data
	UMaterial* GetMaterial(int32 MaterialIndex) const;
	void SetMaterial(int32 MaterialIndex, UMaterial* Material);
	int32 GetNumMaterials() const;
	const TArray<FMeshSection>& GetSections() const;

	// 유효성 검사
	bool IsValid() const { return StaticMeshAsset != nullptr; }

private:
	void EnsureTriangleBVH() const;

	// 실제 데이터 본체(FStaticMesh)에 대한 비소유(non-owning) 포인터.
	// 이 데이터의 실제 소유권 및 생명주기는 AssetManager가 책임집니다.
	TObjectPtr<FStaticMesh> StaticMeshAsset;

	// LOD Meshes (LOD0 = 원본, LOD1 = 50%, LOD2 = 25%)
	TArray<FStaticMesh*> LODMeshes;

	TArray<UMaterial*> Materials;
};
