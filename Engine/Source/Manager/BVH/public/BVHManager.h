#pragma once
#include "Core/Public/Object.h"
#include "Global/Types.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/BatchLines.h"
#include "Editor/Public/ObjectPicker.h"
#include "Physics/Public/AABB.h"
#include "Manager/BVH/public/BVHDebugDraw.h"

struct FBVHNode
{
	FAABB Bounds;
	int LeftChild = -1;
	int RightChild = -1;
	int Start = 0;   // leaf start index
	int Count = 0;   // leaf count
	bool bIsLeaf = false;
};

struct FBVHPrimitive
{
	FVector Center;
	FAABB Bounds;
	UPrimitiveComponent* Primitive;
};

class UBVHManager : UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UBVHManager, UObject)

public:
	void Initialize();

	void Build(const TArray<FBVHPrimitive>& InPrimitives, int MaxLeafSize = 2);
	// void QueryFrustum(const Frustum& frustum, TArray<int>& outVisible) const;
	bool Raycast(const FRay& InRay, UPrimitiveComponent*& HitComponent, float& HitT) const;
	void Refit();
	void SetDebugDrawEnabled(bool bEnabled);
	bool IsDebugDrawEnabled() const { return bDebugDrawEnabled; }
	void RenderDebug(const TArray<FAABB>& InBoxes) const;
	void ConvertComponentsToPrimitives(const TArray<TObjectPtr<UPrimitiveComponent>>& InComponents, TArray<FBVHPrimitive>& OutPrimitives);
	[[nodiscard]] const TArray<FBVHNode>& GetNodes() const { return Nodes; }

	TArray<FAABB>& GetBoxes() { return Boxes; }

private:
	int BuildRecursive(int Start, int Count, int MaxLeafSize);
	FAABB RefitRecursive(int NodeIndex);
	// void QueryRecursive(int nodeIdx, const Frustum& frustum, TArray<int>& outVisible) const;
	void RaycastRecursive(int NodeIndex, const FRay& InRay, float& OutClosestHit, int& OutHitObject) const;
	void RefreshDebugDraw();
	void CollectNodeBounds(TArray<FAABB>& OutBounds) const;

	TArray<FBVHNode> Nodes;
	TArray<FBVHPrimitive> Primitives;
	int RootIndex = -1;
	UBVHDebugDraw DebugDraw;
	bool bDebugDrawEnabled = true;

	TArray<FAABB> Boxes;

	UObjectPicker ObjectPicker;
};
