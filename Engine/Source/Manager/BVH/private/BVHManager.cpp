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

	__m128 var = _mm_setzero_ps();
	__m128 mean = _mm_setr_ps(Mean.X, Mean.Y, Mean.Z, 0.0f);

	for (int i = 0; i < Count; i++) {
		__m128 c = _mm_setr_ps(Primitives[Start + i].Center.X,
							   Primitives[Start + i].Center.Y,
							   Primitives[Start + i].Center.Z, 0.0f);
		__m128 d = _mm_sub_ps(c, mean);
		var = _mm_add_ps(var, _mm_mul_ps(d,d));
	}

	alignas(16) float tmp[4];
	_mm_store_ps(tmp, var);
	Var = FVector(tmp[0], tmp[1], tmp[2]);

	int Axis = 0;
	if (Var.Y > Var.X) Axis = 1;
	if (Var.Z > Var[Axis]) Axis = 2;

	int Mid = Start + Count / 2;
	// 4. Sort primitives along chosen axis
	std::nth_element(
	Primitives.begin() + Start,
	Primitives.begin() + Mid,
	Primitives.begin() + Start + Count,
	[Axis](const FBVHPrimitive& A, const FBVHPrimitive& B) {
		return A.Center[Axis] < B.Center[Axis];
	});

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
	// RaycastIterative(InRay, HitT, HitObjectIndex);
	if (HitObjectIndex == -1)
	{
		return false;
	}

	HitComponent = Primitives[HitObjectIndex].Primitive;
	return true;
}

void UBVHManager::RaycastRecursive(int NodeIndex, const FRay& InRay, float& OutClosestHit, int& OutHitObject) const
{
	const FBVHNode& Node = Nodes[NodeIndex];

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
			if (!Prim.Bounds.RaycastHit(InRay, &t)) continue;
			if (!ObjectPicker.DoesRayIntersectPrimitive_MollerTrumbore(InRay, Prim.Primitive, &t)) continue;
			if (t < OutClosestHit)
			{
				OutClosestHit = t;
				OutHitObject = Node.Start + i;
			}
			break;
		}
	}
	else
	{
		RaycastRecursive(Node.LeftChild, InRay, OutClosestHit, OutHitObject);
		RaycastRecursive(Node.RightChild, InRay, OutClosestHit, OutHitObject);
	}
}

void UBVHManager::RaycastIterative(const FRay& InRay, float& OutClosestHit, int& OutHitObject) const
{
    struct StackEntry { int NodeIndex; float tmin; };
    StackEntry stack[64]; // depth is ~log2(N), 64 is plenty for 50k
    int stackPtr = 0;

    stack[stackPtr++] = { RootIndex, 0.0f };

    while (stackPtr > 0)
    {
        // Pop
        StackEntry entry = stack[--stackPtr];
        int nodeIndex = entry.NodeIndex;

        if (nodeIndex < 0 || nodeIndex >= (int)Nodes.size())
            continue;

        const FBVHNode& Node = Nodes[nodeIndex];

        float tmin;
        if (!Node.Bounds.RaycastHit(InRay, &tmin))
            continue;
        if (tmin > OutClosestHit) // farther than known closest
            continue;

        if (Node.bIsLeaf)
        {
            for (int i = 0; i < Node.Count; i++)
            {
                const FBVHPrimitive& Prim = Primitives[Node.Start + i];
                float t;
                if (Prim.Bounds.RaycastHit(InRay, &t) &&
                    ObjectPicker.DoesRayIntersectPrimitive_MollerTrumbore(InRay, Prim.Primitive, &t))
                {
                    if (t < OutClosestHit)
                    {
                        OutClosestHit = t;
                        OutHitObject  = Node.Start + i;
                    }
                }
            }
        }
        else
        {
            // Check both children
            float tLeft, tRight;
            bool hitLeft  = Nodes[Node.LeftChild].Bounds.RaycastHit(InRay, &tLeft);
            bool hitRight = Nodes[Node.RightChild].Bounds.RaycastHit(InRay, &tRight);

            if (hitLeft && hitRight)
            {
                // Push far child first, near child last (so near is popped first)
                if (tLeft < tRight)
                {
                    if (tRight < OutClosestHit)
                        stack[stackPtr++] = { Node.RightChild, tRight };
                    if (tLeft < OutClosestHit)
                        stack[stackPtr++] = { Node.LeftChild, tLeft };
                }
                else
                {
                    if (tLeft < OutClosestHit)
                        stack[stackPtr++] = { Node.LeftChild, tLeft };
                    if (tRight < OutClosestHit)
                        stack[stackPtr++] = { Node.RightChild, tRight };
                }
            }
            else if (hitLeft && tLeft < OutClosestHit)
            {
                stack[stackPtr++] = { Node.LeftChild, tLeft };
            }
            else if (hitRight && tRight < OutClosestHit)
            {
                stack[stackPtr++] = { Node.RightChild, tRight };
            }
        }
    }
}


void UBVHManager::ConvertComponentsToPrimitives(
	const TArray<TObjectPtr<UPrimitiveComponent>>& InComponents, TArray<FBVHPrimitive>& OutPrimitives)
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

void UBVHManager::FrustumCull(FFrustumCull& InFrustum, TArray<UPrimitiveComponent*>& OutVisibleComponents)
{
	TraverseForCulling(1, InFrustum, OutVisibleComponents);
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

void UBVHManager::TraverseForCulling(uint32 NodeIndex, FFrustumCull& InFrustum,
	TArray<UPrimitiveComponent*>& OutVisibleComponents)
{
	/*
	 *	순회 하기
	 *	현재 노드 AABB 검사 - 완전히 밖이면 culling
	 *	교체 시 - 자식 노드 검사
	 *
	 */
}

