#include "pch.h"
#include "Manager/BVH/public/BVHManager.h"

#include "Editor/Public/ObjectPicker.h"
IMPLEMENT_SINGLETON_CLASS_BASE(UBVHManager)

UBVHManager::UBVHManager() : Boxes()
{
}
UBVHManager::~UBVHManager() = default;

void UBVHManager::Initialize()
{

}

void UBVHManager::Build(const TArray<FBVHPrimitive>& InPrimitives, int MaxLeafSize)
{
	Nodes.clear();
	Primitives = InPrimitives;

	if (Primitives.empty())
	{
		RootIndex = -1;

		return;
	}

	RootIndex = BuildRecursive(0, static_cast<int>(Primitives.size()), MaxLeafSize);
}

int UBVHManager::BuildRecursive(int Start, int Count, int MaxLeafSize)
{
	FBVHNode Node;

	// 1. Compute bounds for this node
	FAABB Bounds = FAABB(
		FVector(+FLT_MAX, +FLT_MAX, +FLT_MAX),
		FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX)
		);
	for (int i = 0; i < Count; i++)
	{
		int Index = Start + i;
		FAABB primitiveBounds = Primitives[Index].Bounds;
		Bounds = Bounds.Union(Bounds, primitiveBounds);
	}
	Node.Bounds = Bounds;
	// UE_LOG("Bounds now: (%f, %f), (%f, %f)", Bounds.Min.X, Bounds.Max.X, Bounds.Min.Y, Bounds.Max.Y);

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
		Mean += Primitives[Start + i].Center;
	Mean /= (float)Count;

	for (int i = 0; i < Count; i++)
	{
		FVector d = Primitives[Start + i].Center - Mean;
		Var.X += d.X * d.X;
		Var.Y += d.Y * d.Y;
		Var.Z += d.Z * d.Z;
	}

	int Axis = 0;
	if (Var.Y > Var.X) Axis = 1;
	if (Var.Z > Var[Axis]) Axis = 2;

	// 4. Sort primitives along chosen axis
	std::sort(
	Primitives.begin() + Start,
	Primitives.begin() + Start + Count,
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

void UBVHManager::Refit()
{
	if (Nodes.empty() || Primitives.empty())
	{
		RootIndex = -1;
		return;
	}

	// Step 1: Update primitive bounds from their components
	for (FBVHPrimitive& Prim : Primitives)
	{
		if (!Prim.Primitive || !Prim.Primitive->IsVisible())
			continue;

		FVector WorldMin, WorldMax;
		Prim.Primitive->GetWorldAABB(WorldMin, WorldMax);
		Prim.Bounds = FAABB(WorldMin, WorldMax);
		Prim.Center = (WorldMin + WorldMax) * 0.5f;
	}

	// Step 2: Recompute node bounds bottom-up
	RefitRecursive(RootIndex);
}


FAABB UBVHManager::RefitRecursive(int NodeIndex)
{
	FBVHNode& Node = Nodes[NodeIndex];

	if (Node.bIsLeaf)
	{
		FAABB Bounds(
			FVector(+FLT_MAX, +FLT_MAX, +FLT_MAX),
			FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX)
		);

		for (int i = 0; i < Node.Count; i++)
		{
			int primIndex = Node.Start + i;
			Bounds = Bounds.Union(Bounds, Primitives[primIndex].Bounds);
		}

		Node.Bounds = Bounds;
		return Bounds;
	}
	else
	{
		FAABB LeftBounds = RefitRecursive(Node.LeftChild);
		FAABB RightBounds = RefitRecursive(Node.RightChild);

		Node.Bounds = Node.Bounds.Union(LeftBounds, RightBounds);
		return Node.Bounds;
	}
}

bool UBVHManager::Raycast(const FRay& InRay, UPrimitiveComponent*& HitComponent, float& HitT) const
{
	HitComponent = nullptr;

	if (RootIndex < 0 || Nodes.empty())
		return false;

	HitT = FLT_MAX;
	int HitObjectIndex = -1;

	RaycastRecursive(RootIndex, InRay, HitT, HitObjectIndex);
	if (HitObjectIndex == -1)
	{
		UE_LOG("Raycast: No hit");
		return false;
	}

	HitComponent = Primitives[HitObjectIndex].Primitive;
	UE_LOG("Raycast: Hit Object Index=%d", HitObjectIndex);
	return true;
}

void UBVHManager::RaycastRecursive(int NodeIndex, const FRay& InRay, float& OutClosestHit, int& OutHitObject) const
{
	if (NodeIndex < 0 || NodeIndex >= (int)Nodes.size())
	{
		UE_LOG("Invalid NodeIndex: %d (Nodes.size()=%d)", NodeIndex, (int)Nodes.size());
		return;
	}

	const FBVHNode& Node = Nodes[NodeIndex];

	// UE_LOG("BuildRecursive: Start=%d Count=%d -> NodeIndex=%d Leaf=%d",
	//    Node.Start, Node.Count, NodeIndex, Node.bIsLeaf);

	float tmin;
	if (!Node.Bounds.RaycastHit(InRay, &tmin))
		return;

	// Prune farther intersections
	if (tmin > OutClosestHit)
		return;

	if (Node.bIsLeaf)
	{
		for (int i = 0; i < Node.Count; i++)
		{
			const FBVHPrimitive& Prim = Primitives[Node.Start + i];
			float t;
			if (Prim.Bounds.RaycastHit(InRay, &t)
				&& ObjectPicker.DoesRayIntersectPrimitive_MollerTrumbore(InRay, Prim.Primitive, &t))
			{
				if (t < OutClosestHit)
				{
					OutClosestHit = t;
					OutHitObject = Node.Start + i;
				}
			}
		}
	}
	else
	{
		RaycastRecursive(Node.LeftChild, InRay, OutClosestHit, OutHitObject);
		RaycastRecursive(Node.RightChild, InRay, OutClosestHit, OutHitObject);
	}
}


void UBVHManager::ConvertComponentsToPrimitives(const TArray<TObjectPtr<UPrimitiveComponent>>& InComponents, TArray<FBVHPrimitive>& OutPrimitives)
{
	OutPrimitives.clear();
	OutPrimitives.reserve(InComponents.size());

	for (UPrimitiveComponent* Component : InComponents)
	{
		if (!Component || !Component->IsVisible())
		{
			continue;
		}
		FVector WorldMin, WorldMax;
		Component->GetWorldAABB(WorldMin, WorldMax);

		FBVHPrimitive Primitive;
		Primitive.Bounds = FAABB(WorldMin, WorldMax);
		Primitive.Center = (WorldMin + WorldMax) * 0.5f;
		Primitive.Primitive = Component;

		OutPrimitives.push_back(Primitive);
	}
}

void UBVHManager::CollectNodeBounds(TArray<FAABB>& OutBounds) const
{
	OutBounds.clear();
	OutBounds.reserve(Nodes.size());

	for (const FBVHNode& Node : Nodes)
	{
		OutBounds.push_back(Node.Bounds);
	}
}

