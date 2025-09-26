#pragma once
#include "Core/Public/Object.h"
#include "Global/Types.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Physics/Public/AABB.h"

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
public:
	void Build(TArray<FBVHPrimitive>& InPrimitives, int MaxLeafSize = 4);
	// void QueryFrustum(const Frustum& frustum, TArray<int>& outVisible) const;
	bool Raycast(const FRay& InRay, int& HitObject, float& HitT) const;

private:
	int BuildRecursive(int Start, int Count, int MaxLeafSize);
	// void QueryRecursive(int nodeIdx, const Frustum& frustum, TArray<int>& outVisible) const;
	void RaycastRecursive(int NodeIndex, const FRay& InRay, float& OutClosestHit, int& OutHitObject) const;

	TArray<FBVHNode> Nodes;
	TArray<FBVHPrimitive>* Primitives; // pointer to external array
	int RootIndex = -1;
};
