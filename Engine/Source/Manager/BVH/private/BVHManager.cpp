#include "pch.h"
#include "Manager/BVH/public/BVHManager.h"

#include "Core/Public/ScopeCycleCounter.h"
#include "Editor/Public/ObjectPicker.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"

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
		Prim.WorldToModel = Prim.Primitive->GetWorldTransformMatrixInverse();
		Prim.PrimitiveType = Prim.Primitive->GetPrimitiveType();
		Prim.StaticMesh = nullptr;

		if (Prim.PrimitiveType == EPrimitiveType::StaticMesh)
		{
			if (auto* StaticMeshComponent = static_cast<UStaticMeshComponent*>(Prim.Primitive))
			{
				Prim.StaticMesh = StaticMeshComponent->GetStaticMesh();
			}
		}
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
    {
        return false;
    }

    HitT = FLT_MAX;
    int HitObjectIndex = -1;

    RaycastIterative(InRay, HitT, HitObjectIndex);
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

    float tmin = 0.0f;
    if (!Node.Bounds.RaycastHit(InRay, &tmin) || tmin > OutClosestHit)
    {
        return;
    }

    if (Node.bIsLeaf)
    {
        for (int i = 0; i < Node.Count; ++i)
        {
            const FBVHPrimitive& Prim = Primitives[Node.Start + i];
            if (!Prim.Primitive || !Prim.Primitive->IsVisible())
            {
                continue;
            }

            float boxT = 0.0f;
            if (!Prim.Bounds.RaycastHit(InRay, &boxT) || boxT > OutClosestHit)
            {
                continue;
            }

            float candidateDistance = OutClosestHit;
            bool bHitPrimitive = false;

            if (Prim.PrimitiveType == EPrimitiveType::StaticMesh && Prim.StaticMesh)
            {
                FRay ModelRay;
                ModelRay.Origin = InRay.Origin * Prim.WorldToModel;
                ModelRay.Direction = InRay.Direction * Prim.WorldToModel;
                ModelRay.Direction.Normalize();

                bHitPrimitive = Prim.StaticMesh->RaycastTriangleBVH(ModelRay, candidateDistance);
            }
            else
            {
                bHitPrimitive = ObjectPicker.DoesRayIntersectPrimitive_MollerTrumbore(InRay, Prim.Primitive, &candidateDistance);
            }

            if (bHitPrimitive && candidateDistance < OutClosestHit)
            {
                OutClosestHit = candidateDistance;
                OutHitObject = Node.Start + i;
                break;
            }
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
    struct FStackEntry
    {
        int NodeIndex;
        float Distance;
    };

    FStackEntry stack[64];
    int stackPtr = 0;
    const int stackCapacity = static_cast<int>(sizeof(stack) / sizeof(stack[0]));

    auto Push = [&](int node, float distance)
    {
        if (stackPtr < stackCapacity)
        {
            stack[stackPtr++] = { node, distance };
        }
    };

    Push(RootIndex, 0.0f);

    while (stackPtr > 0)
    {
        const FStackEntry entry = stack[--stackPtr];
        const int nodeIndex = entry.NodeIndex;

        if (nodeIndex < 0 || nodeIndex >= static_cast<int>(Nodes.size()))
        {
            continue;
        }

        const FBVHNode& Node = Nodes[nodeIndex];

        float tmin = 0.0f;
        if (!Node.Bounds.RaycastHit(InRay, &tmin) || tmin > OutClosestHit)
        {
            continue;
        }

        if (Node.bIsLeaf)
        {
            for (int i = 0; i < Node.Count; ++i)
            {
                const FBVHPrimitive& Prim = Primitives[Node.Start + i];
                if (!Prim.Primitive || !Prim.Primitive->IsVisible())
                {
                    continue;
                }

                float boxT = 0.0f;
                if (!Prim.Bounds.RaycastHit(InRay, &boxT) || boxT > OutClosestHit)
                {
                    continue;
                }

                float candidateDistance = OutClosestHit;
                bool bHitPrimitive = false;

                if (Prim.PrimitiveType == EPrimitiveType::StaticMesh && Prim.StaticMesh)
                {
                    FRay ModelRay;
                    ModelRay.Origin = InRay.Origin * Prim.WorldToModel;
                    ModelRay.Direction = InRay.Direction * Prim.WorldToModel;
                    ModelRay.Direction.Normalize();

                    bHitPrimitive = Prim.StaticMesh->RaycastTriangleBVH(ModelRay, candidateDistance);
                }
                else
                {
                    bHitPrimitive = ObjectPicker.DoesRayIntersectPrimitive_MollerTrumbore(InRay, Prim.Primitive, &candidateDistance);
                }

                if (bHitPrimitive && candidateDistance < OutClosestHit)
                {
                    OutClosestHit = candidateDistance;
                    OutHitObject = Node.Start + i;
                    break;
                }
            }
        }
        else
        {
            const int leftChild = Node.LeftChild;
            const int rightChild = Node.RightChild;

            float leftDistance = 0.0f;
            const bool hitLeft = (leftChild != -1) && Nodes[leftChild].Bounds.RaycastHit(InRay, &leftDistance) && leftDistance <= OutClosestHit;

            float rightDistance = 0.0f;
            const bool hitRight = (rightChild != -1) && Nodes[rightChild].Bounds.RaycastHit(InRay, &rightDistance) && rightDistance <= OutClosestHit;

            if (hitLeft && hitRight)
            {
                if (leftDistance < rightDistance)
                {
                    Push(rightChild, rightDistance);
                    Push(leftChild, leftDistance);
                }
                else
                {
                    Push(leftChild, leftDistance);
                    Push(rightChild, rightDistance);
                }
            }
            else if (hitLeft)
            {
                Push(leftChild, leftDistance);
            }
            else if (hitRight)
            {
                Push(rightChild, rightDistance);
            }
        }
    }
}
void UBVHManager::ConvertComponentsToBVHPrimitives(
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
		Primitive.WorldToModel = Component->GetWorldTransformMatrixInverse();
		Primitive.PrimitiveType = Component->GetPrimitiveType();
		Primitive.StaticMesh = nullptr;

		if (Primitive.PrimitiveType == EPrimitiveType::StaticMesh)
		{
			if (auto* StaticMeshComponent = static_cast<UStaticMeshComponent*>(Component))
			{
				Primitive.StaticMesh = StaticMeshComponent->GetStaticMesh();
			}
		}

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
