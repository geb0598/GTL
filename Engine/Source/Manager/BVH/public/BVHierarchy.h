#pragma once
#include "Core/Public/Object.h"
#include "Global/Types.h"
#include "Global/Matrix.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/BatchLines.h"
#include "Editor/Public/ObjectPicker.h"
#include "Physics/Public/AABB.h"

class UStaticMesh;
struct FBVHNode
{
	FAABB Bounds;
	int LeftChild = -1;
	int RightChild = -1;
	int Start = 0;   // leaf start index
	int Count = 0;   // leaf count
	bool bIsLeaf = false;

	uint32 FrustumMask = 0;
};

// deprecated
struct TriBVHNode {
	FAABB Bounds;
	int LeftChild;    // -1 if leaf
	int RightChild;   // -1 if leaf
	int Start;        // index into triangle array
	int Count;        // number of triangles in leaf
	bool bIsLeaf;
};

struct FBVHPrimitive
{
	FVector Center;
	FAABB Bounds;
	TObjectPtr<UPrimitiveComponent> Primitive;
	FMatrix WorldToModel;
	EPrimitiveType PrimitiveType = EPrimitiveType::Cube;
	UStaticMesh* StaticMesh = nullptr;
};

struct FBin
{
	FAABB Bounds = FAABB(FVector(+FLT_MAX, +FLT_MAX, +FLT_MAX),
					FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX));;
	int PrimitiveCount = 0;
};

struct FSplitInfo
{
	float Cost = FLT_MAX;
	uint32 Axis = 99;
	uint32 BinIndex = -1;
	FSplitInfo() = default;
	FSplitInfo(float InCost, uint32 InAxis, uint32 InBinIndex)
		: Cost(InCost), Axis(InAxis), BinIndex(InBinIndex) {}
	FSplitInfo(const FSplitInfo& Other)
	{
		this->Cost = Other.Cost;
		this->Axis = Other.Axis;
		this->BinIndex = Other.BinIndex;
	}
};

struct FPrimitiveLength
{
	int Start = 0;
	int Count = 0;
	FPrimitiveLength() = default;
	FPrimitiveLength(int InStart, int InCount) : Start(InStart), Count(InCount) {}
	FPrimitiveLength(const FPrimitiveLength& Other)
	{
		Start = Other.Start;
		Count = Other.Count;
	}
};

struct FBuildContext
{
	FPrimitiveLength Length;
	FVector CentroidExtent;
	FVector CentroidMin;
};

class FFrustumCull;

class UBVHierarchy : UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UBVHierarchy, UObject)

public:
	void Initialize();

	void Build(const TArray<FBVHPrimitive>& InPrimitives, int MaxLeafSize = 5);
	// void QueryFrustum(const Frustum& frustum, TArray<int>& outVisible) const;
	bool Raycast(const FRay& InRay, UPrimitiveComponent*& HitComponent, float& HitT) const;
	void Refit();
	bool IsDebugDrawEnabled() const { return bDebugDrawEnabled; }
	void ConvertComponentsToBVHPrimitives(const TArray<TObjectPtr<UPrimitiveComponent>>& InComponents, TArray<FBVHPrimitive>& OutPrimitives);
	[[nodiscard]] const TArray<FBVHNode>& GetNodes() const { return Nodes; }
	void FrustumCull(FFrustumCull& InFrustum, TArray<TObjectPtr<UPrimitiveComponent>>& OutVisibleComponents);

	TArray<FAABB>& GetBoxes() { return Boxes; }

private:
	int BuildRecursive(FPrimitiveLength Length, int MaxLeafSize);
	FAABB RefitRecursive(int NodeIndex);
	// void QueryRecursive(int nodeIdx, const Frustum& frustum, TArray<int>& outVisible) const;
	void RaycastIterative(const FRay& InRay, float& OutClosestHit, int& OutHitObject) const;
	void RaycastRecursive(int NodeIndex, const FRay& InRay, float& OutClosestHit, int& OutHitObject) const;
	void CollectNodeBounds(TArray<FAABB>& OutBounds) const;
	void TraverseForCulling(uint32 NodeIndex, FFrustumCull& InFrustum, uint32 InMask, TArray<TObjectPtr<UPrimitiveComponent>>& OutVisibleComponents);
	void AddAllPrimitives(uint32 NodeIndex, TArray<TObjectPtr<UPrimitiveComponent>>& OutVisibleComponents);

	// 헬퍼 함수
	int CreateLeafNode(FPrimitiveLength Length, FBVHNode& LeafNode);
	inline void CalculateCurrentNodeBounds(FPrimitiveLength Length, FBVHNode& CurrentNode) const;
	inline FVector CalculateCentroid(FVector& CenteroidMax, FVector& CenteroidMin, FPrimitiveLength Length) const;
	uint32 PartitionPrimitives(FSplitInfo& Split, const uint32 NumBins, const FBuildContext& Context);
	FSplitInfo FindBestSplit(const FAABB& CurrentBounds, TArray<FBin>& Bins, const FBuildContext& Context) const;


private:
	UObjectPicker ObjectPicker;

	TArray<FBVHNode> Nodes;
	TArray<FBVHPrimitive> Primitives;
	int RootIndex = -1;
	bool bDebugDrawEnabled = true;

	TArray<FAABB> Boxes;
};

inline FVector UBVHierarchy::CalculateCentroid(FVector& CenteroidMax, FVector& CenteroidMin, FPrimitiveLength Length) const
{
	for (int i = Length.Start; i < Length.Start + Length.Count; i++)
	{
		CenteroidMin.X = std::min(CenteroidMin.X, Primitives[i].Center.X);
		CenteroidMin.Y = std::min(CenteroidMin.Y, Primitives[i].Center.Y);
		CenteroidMin.Z = std::min(CenteroidMin.Z, Primitives[i].Center.Z);
		CenteroidMax.X = std::max(CenteroidMax.X, Primitives[i].Center.X);
		CenteroidMax.Y = std::max(CenteroidMax.Y, Primitives[i].Center.Y);
		CenteroidMax.Z = std::max(CenteroidMax.Z, Primitives[i].Center.Z);
	}

	return CenteroidMax - CenteroidMin;
}

inline void UBVHierarchy::CalculateCurrentNodeBounds(FPrimitiveLength Length, FBVHNode& CurrentNode) const
{
	FAABB Bounds = FAABB::GetEmptyAABB();
	// 현재 노드의 BV 계산
	for (uint32 i = Length.Start; i < Length.Start + Length.Count; i++)
	{
		Bounds = Bounds.Union(Bounds, Primitives[i].Bounds);
	}
	CurrentNode.Bounds = Bounds;
}

