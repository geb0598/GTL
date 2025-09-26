#include "pch.h"
#include "Manager/BVH/public/BVHManager.h"

#include "Editor/Public/BatchLines.h"

void UBVHManager::Build(TArray<FBVHPrimitive>& InPrimitives, int MaxLeafSize)
{
	Nodes.clear();
	Primitives = &InPrimitives;

	if (Primitives->size() == 0)
	{
		RootIndex = -1;
		return;
	}

	RootIndex = BuildRecursive(0, Primitives->size(), MaxLeafSize);
}

int UBVHManager::BuildRecursive(int Start, int Count, int MaxLeafSize)
{
	FBVHNode Node;

	// 1. Compute bounds for this node
	FAABB Bounds = FAABB();
	for (int i = 0; i < Count; i++)
	{
		int Index = Start + i;
		Bounds = Bounds.Union(Bounds, (*Primitives)[Index].Bounds);
	}
	Node.Bounds = Bounds;



	// 2. Leaf condition
	if (Count <= MaxLeafSize)
	{
		Node.bIsLeaf = true;
		Node.Start = Start;
		Node.Count = Count;

		int NodeIndex = Nodes.size();
		Nodes.push_back(Node);
		return NodeIndex;
	}

	// 3. Choose split axis (largest variance of centers)
	FVector Mean(0,0,0), Var(0,0,0);
	for (int i = 0; i < Count; i++)
		Mean += (*Primitives)[Start + i].Center;
	Mean /= (float)Count;

	for (int i = 0; i < Count; i++)
	{
		FVector d = (*Primitives)[Start + i].Center - Mean;
		Var.X += d.X * d.X;
		Var.Y += d.Y * d.Y;
		Var.Z += d.Z * d.Z;
	}

	int Axis = 0;
	if (Var.Y > Var.X) Axis = 1;
	if (Var.Z > Var[Axis]) Axis = 2;

	// 4. Sort primitives along chosen axis
	std::sort(
	Primitives->begin() + Start,
	Primitives->begin() + Start + Count,
	[Axis](const FBVHPrimitive& A, const FBVHPrimitive& B)
		{
			return A.Center[Axis] < B.Center[Axis];
		}
	);

	int Mid = Start + Count / 2;

	// 5. Recurse children
	int LeftIndex = BuildRecursive(Start, Mid - Start, MaxLeafSize);
	int RightIndex = BuildRecursive(Mid, Count - (Mid - Start), MaxLeafSize);

	Node.LeftChild = LeftIndex;
	Node.RightChild = RightIndex;

	int NodeIndex = Nodes.size();
	Nodes.push_back(Node);
	return NodeIndex;
}

bool UBVHManager::Raycast(const FRay& InRay, int& HitObject, float& HitT) const
{

}

void UBVHManager::RaycastRecursive(int NodeIndex, const FRay& InRay, float& OutClosestHit, int& OutHitObject) const
{

}

