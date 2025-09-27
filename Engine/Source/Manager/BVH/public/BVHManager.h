#pragma once
#include "Core/Public/Object.h"
#include "Global/Types.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/BatchLines.h"
#include "Editor/Public/ObjectPicker.h"
#include "Physics/Public/AABB.h"

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

struct FBVHPrimitive
{
	FVector Center;
	FAABB Bounds;
	UPrimitiveComponent* Primitive;
};

class FFrustumCull;

class UBVHManager : UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UBVHManager, UObject)

public:
	void Initialize();

	void Build(const TArray<FBVHPrimitive>& InPrimitives, int MaxLeafSize = 5);
	// void QueryFrustum(const Frustum& frustum, TArray<int>& outVisible) const;
	bool Raycast(const FRay& InRay, UPrimitiveComponent*& HitComponent, float& HitT) const;
	void Refit();
	bool IsDebugDrawEnabled() const { return bDebugDrawEnabled; }
	void ConvertComponentsToPrimitives(const TArray<TObjectPtr<UPrimitiveComponent>>& InComponents, TArray<FBVHPrimitive>& OutPrimitives);
	[[nodiscard]] const TArray<FBVHNode>& GetNodes() const { return Nodes; }
	void FrustumCull(FFrustumCull& InFrustum, TArray<UPrimitiveComponent*>& OutVisibleComponents);

	TArray<FAABB>& GetBoxes() { return Boxes; }

private:
	int BuildRecursive(int Start, int Count, int MaxLeafSize);
	FAABB RefitRecursive(int NodeIndex);
	// void QueryRecursive(int nodeIdx, const Frustum& frustum, TArray<int>& outVisible) const;
	void RaycastIterative(const FRay& InRay, float& OutClosestHit, int& OutHitObject) const;
	void RaycastRecursive(int NodeIndex, const FRay& InRay, float& OutClosestHit, int& OutHitObject) const;
	void CollectNodeBounds(TArray<FAABB>& OutBounds) const;
	void TraverseForCulling(uint32 NodeIndex, FFrustumCull& InFrustum, TArray<UPrimitiveComponent*>& OutVisibleComponents);

	TArray<FBVHNode> Nodes;
	TArray<FBVHPrimitive> Primitives;
	int RootIndex = -1;
	bool bDebugDrawEnabled = true;

	TArray<FAABB> Boxes;

	UObjectPicker ObjectPicker;
};
