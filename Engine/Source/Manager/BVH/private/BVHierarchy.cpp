#include "pch.h"
#include "Manager/BVH/public/BVHierarchy.h"

#include "Editor/Public/FrustumCull.h"

#include "Core/Public/ScopeCycleCounter.h"
#include "Editor/Public/ObjectPicker.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UBVHierarchy)

UBVHierarchy::UBVHierarchy() : Boxes()
{
}
UBVHierarchy::~UBVHierarchy() = default;

void UBVHierarchy::Initialize()
{
}

void UBVHierarchy::Build(const TArray<FBVHPrimitive>& InPrimitives, int MaxLeafSize)
{
	Nodes.clear();
	Primitives = InPrimitives;

	if (Primitives.empty())
	{
		RootIndex = -1;

		return;
	}

	FPrimitiveLength Length = {0, static_cast<int>(Primitives.size())};
	RootIndex = BuildRecursive(Length, MaxLeafSize);
}

int UBVHierarchy::BuildRecursive(FPrimitiveLength Length, int MaxLeafSize)
{
	FBVHNode CurrentNode{};

	CalculateCurrentNodeBounds(Length, CurrentNode);

	if (Length.Count <= MaxLeafSize)
	{
		return CreateLeafNode(Length, CurrentNode);
	}

	FVector CentroidMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	FVector CentroidMin(FLT_MAX, FLT_MAX, FLT_MAX);
	FVector CentroidExtent = CalculateCentroid(CentroidMax, CentroidMin, Length);

	FBuildContext Context = {Length, CentroidExtent, CentroidMin};

	constexpr uint32 NumBins = 16;
	TArray<FBin> Bins(NumBins);
	FSplitInfo BestSplit = FindBestSplit(CurrentNode.Bounds, Bins, Context);

	// 분할하지 않는 비용은 프리미티브 개수로 가정한다.
	if (BestSplit.Cost > static_cast<float>(Length.Count) || BestSplit.Axis == 99)
	{
		// 분할이 비효율적이거나 유효하지 않는 경우 leaf노드 생성
		return CreateLeafNode(Length, CurrentNode);
	}


	int MidIndex = PartitionPrimitives(BestSplit, NumBins, Context);

	FPrimitiveLength LeftLength = {Length.Start, (MidIndex - Length.Start)};
	uint32 LeftChild = BuildRecursive(LeftLength, MaxLeafSize);

	FPrimitiveLength RightLength = {MidIndex, (Length.Start + Length.Count - MidIndex)};
	uint32 RightChild = BuildRecursive(RightLength, MaxLeafSize);

	CurrentNode.bIsLeaf = false;
	CurrentNode.LeftChild = LeftChild;
	CurrentNode.RightChild = RightChild;

	Nodes.push_back(CurrentNode);
	return (Nodes.size() - 1);


#pragma region original
	// FBVHNode Node;
	//
	// // 1. Compute bounds for this node
	// FAABB Bounds = FAABB(
	// 	FVector(+FLT_MAX, +FLT_MAX, +FLT_MAX),
	// 	FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX)
	// 	);
	// for (int i = 0; i < Count; i++)
	// {
	// 	int Index = Start + i;
	// 	FAABB primitiveBounds = Primitives[Index].Bounds;
	// 	Bounds = Bounds.Union(Bounds, primitiveBounds);
	// }
	// Node.Bounds = Bounds;
	//
	// // 2. Leaf condition
	// if (Count <= MaxLeafSize)
	// {
	// 	Node.bIsLeaf = true;
	// 	Node.Start = Start;
	// 	Node.Count = Count;
	//
	// 	int NodeIndex = Nodes.size();
	// 	Nodes.push_back(Node);
	// 	return NodeIndex;
	// }
	//
	// // 3. Choose split axis (largest variance of centers)
	// FVector Mean(0,0,0), Var(0,0,0);
	// for (int i = 0; i < Count; i++)
	// 	Mean += Primitives[Start + i].Center;
	// Mean /= (float)Count;
	//
	// __m128 var = _mm_setzero_ps();
	// __m128 mean = _mm_setr_ps(Mean.X, Mean.Y, Mean.Z, 0.0f);
	//
	// for (int i = 0; i < Count; i++) {
	// 	__m128 c = _mm_setr_ps(Primitives[Start + i].Center.X,
	// 						   Primitives[Start + i].Center.Y,
	// 						   Primitives[Start + i].Center.Z, 0.0f);
	// 	__m128 d = _mm_sub_ps(c, mean);
	// 	var = _mm_add_ps(var, _mm_mul_ps(d,d));
	// }
	//
	// alignas(16) float tmp[4];
	// _mm_store_ps(tmp, var);
	// Var = FVector(tmp[0], tmp[1], tmp[2]);
	//
	// int Axis = 0;
	// if (Var.Y > Var.X) Axis = 1;
	// if (Var.Z > Var[Axis]) Axis = 2;
	//
	// int Mid = Start + Count / 2;
	// // 4. Sort primitives along chosen axis
	// std::nth_element(
	// Primitives.begin() + Start,
	// Primitives.begin() + Mid,
	// Primitives.begin() + Start + Count,
	// [Axis](const FBVHPrimitive& A, const FBVHPrimitive& B) {
	// 	return A.Center[Axis] < B.Center[Axis];
	// });
	//
	// // 5. Recurse children
	// int LeftIndex = BuildRecursive(Start, Mid - Start, MaxLeafSize);
	// int RightIndex = BuildRecursive(Mid, Count - (Mid - Start), MaxLeafSize);
	//
	// Node.LeftChild = LeftIndex;
	// Node.RightChild = RightIndex;
	//
	// int NodeIndex = Nodes.size();
	// Nodes.push_back(Node);
	// return NodeIndex;
#pragma endregion
}

void UBVHierarchy::Refit()
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
			if (auto StaticMeshComponent = Cast<UStaticMeshComponent>(Prim.Primitive))
			{
				Prim.StaticMesh = StaticMeshComponent->GetStaticMesh();
			}
		}
	}

	// Step 2: Recompute node bounds bottom-up
	RefitRecursive(RootIndex);
}


FAABB UBVHierarchy::RefitRecursive(int NodeIndex)
{
	FBVHNode& Node = Nodes[NodeIndex];

	if (Node.bIsLeaf)
	{
		FPrimitiveLength Lenght = {Node.Start, Node.Count};
		CalculateCurrentNodeBounds(Lenght, Node);

		FAABB Bounds = FAABB::GetEmptyAABB();

		return Node.Bounds;
	}
	else
	{
		FAABB LeftBounds = RefitRecursive(Node.LeftChild);
		FAABB RightBounds = RefitRecursive(Node.RightChild);

		Node.Bounds = Node.Bounds.Union(LeftBounds, RightBounds);
		return Node.Bounds;
	}
}

bool UBVHierarchy::Raycast(const FRay& InRay, UPrimitiveComponent*& HitComponent, float& HitT) const
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

void UBVHierarchy::RaycastRecursive(int NodeIndex, const FRay& InRay, float& OutClosestHit, int& OutHitObject) const
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

void UBVHierarchy::RaycastIterative(const FRay& InRay, float& OutClosestHit, int& OutHitObject) const
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
void UBVHierarchy::ConvertComponentsToBVHPrimitives(
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

void UBVHierarchy::FrustumCull(FFrustumCull& InFrustum, TArray<TObjectPtr<UPrimitiveComponent>>& OutVisibleComponents)
{
	OutVisibleComponents.clear();

	// 루트가 음수 == BVH 트리가 없다.
	if (RootIndex < 0)
	{
		return;
	}

	TraverseForCulling(RootIndex, InFrustum, ToBaseType(EFrustumPlane::All), OutVisibleComponents);
}

void UBVHierarchy::CollectNodeBounds(TArray<FAABB>& OutBounds) const
{
	OutBounds.clear();
	OutBounds.reserve(Nodes.size());

	for (const FBVHNode& Node : Nodes)
	{
		OutBounds.push_back(Node.Bounds);
	}
}

void UBVHierarchy::TraverseForCulling(uint32 NodeIndex, FFrustumCull& InFrustum, uint32 InMask,
	TArray<TObjectPtr<UPrimitiveComponent>>& OutVisibleComponents)
{
	/*
	 *	BVH의 바운딩박스는 worldbox
	 */

	// 현재 노드의 BB 검사
	FAABB CurrentBound = Nodes[NodeIndex].Bounds;

	TArray<EFrustumPlane> PlaneMasks = { EFrustumPlane::Left, EFrustumPlane::Right,
										EFrustumPlane::Bottom, EFrustumPlane::Top,
										EFrustumPlane::Near, EFrustumPlane::Far};

	TArray<EPlaneIndex> PlaneIndices = { EPlaneIndex::Left, EPlaneIndex::Right,
										 EPlaneIndex::Bottom, EPlaneIndex::Top,
										 EPlaneIndex::Near, EPlaneIndex::Far};



	uint32 ChildMask = 0;
	EFrustumTestResult OverallResult = EFrustumTestResult::CompletelyInside;

	// Near/Far 평면을 먼저 검사 (더 높은 확률로 culling 가능)
	static const int PlaneOrder[6] = { 4, 5, 0, 1, 2, 3 }; // Near, Far, Left, Right, Bottom, Top

	// Plane Test
	for (int idx = 0; idx < 6; idx++)
	{
		int i = PlaneOrder[idx];
		EFrustumPlane CurrentPlaneFlag = static_cast<EFrustumPlane>(1 << i);
		EPlaneIndex CurrentPlaneIndex = static_cast<EPlaneIndex>(i);
		if (InMask & ToBaseType(CurrentPlaneFlag))
		{
			EFrustumTestResult Result = InFrustum.TestAABBWithPlane(CurrentBound, CurrentPlaneIndex);
			// 완전히 바깥인 경우 return
			// 더 이상 검사할 필요가 없음
			if (Result == EFrustumTestResult::CompletelyOutside)
			{
				return;
			}
			else if (Result == EFrustumTestResult::Intersect)
			{
				OverallResult = EFrustumTestResult::Intersect;
				ChildMask |= ToBaseType(PlaneMasks[static_cast<uint32>(CurrentPlaneIndex)]);
			}
		}
	}

	// 완전히 안쪽인 경우
	// 모든 자식의 primitive를 outvisiblecomp에 추가 후 순회 종료
	if (OverallResult == EFrustumTestResult::CompletelyInside)
	{
		AddAllPrimitives(NodeIndex, OutVisibleComponents);
		return;
	}

	// 완전히 바깥도 아니고 완전히 안쪽도 아니면 교차
	// 교차하는 AABB가 Leaf라면 leaf node 내부의 개별 primitive에 대해서 culling test 필요
	if (Nodes[NodeIndex].bIsLeaf)
	{
		size_t Count = Nodes[NodeIndex].Start + Nodes[NodeIndex].Count;
		for (size_t i = Nodes[NodeIndex].Start; i < Count; i++)
		{
			FAABB TargetAABB = Primitives[i].Bounds;
			if (InFrustum.IsInFrustum(TargetAABB) != EFrustumTestResult::CompletelyOutside &&
				Primitives[i].Primitive->IsVisible())
			{
				OutVisibleComponents.push_back(Primitives[i].Primitive);
			}
		}
		return;
	}

	// leaf가 아니고 교차하는 경우 순회 시작
	TraverseForCulling(Nodes[NodeIndex].LeftChild, InFrustum, ChildMask, OutVisibleComponents);
	TraverseForCulling(Nodes[NodeIndex].RightChild, InFrustum, ChildMask, OutVisibleComponents);

}

void UBVHierarchy::AddAllPrimitives(uint32 NodeIndex, TArray<TObjectPtr<UPrimitiveComponent>>& OutVisibleComponents)
{
	if (NodeIndex < 0 || NodeIndex >= Nodes.size())
	{
		return;
	}

	FBVHNode CurrentNode = Nodes[NodeIndex];
	if (CurrentNode.bIsLeaf)
	{
		size_t Count = CurrentNode.Start + CurrentNode.Count;
		for (size_t i = CurrentNode.Start; i < Count; i++)
		{
			if (Primitives[i].Primitive->IsVisible())
			{
				OutVisibleComponents.push_back(Primitives[i].Primitive);
			}
		}
		return;
	}

	// completely inside가 중간노드인 경우 leaf노드로 재귀
	AddAllPrimitives(CurrentNode.LeftChild, OutVisibleComponents);
	AddAllPrimitives(CurrentNode.RightChild, OutVisibleComponents);
}

int UBVHierarchy::CreateLeafNode(FPrimitiveLength Length, FBVHNode& LeafNode)
{
	// leaf노드에 저장된 primtives의 index 저장
	LeafNode.Start = Length.Start;
	LeafNode.Count = Length.Count;
	LeafNode.bIsLeaf = true;

	Nodes.push_back(LeafNode);

	return Nodes.size() - 1;
}

uint32 UBVHierarchy::PartitionPrimitives(FSplitInfo& Split, const uint32 NumBins, const FBuildContext& Context)
{
	int Start = Context.Length.Start;
	int Count = Context.Length.Count;
	FVector CentroidExtent = Context.CentroidExtent;
	FVector CentroidMin = Context.CentroidMin;

	float SplitCoordinate = CentroidMin[Split.Axis] + static_cast<float>(Split.BinIndex) * CentroidExtent[Split.Axis] / NumBins;
	auto MidPrimPointer = std::partition(Primitives.begin() + Start,
		Primitives.begin() + (Start + Count),
		[=](const FBVHPrimitive& Primitive){ return Primitive.Center[Split.Axis] < SplitCoordinate;});
	uint32 MidIndex = std::distance(Primitives.begin(), MidPrimPointer);

	if (MidIndex == Start || (MidIndex == Start + Count))
	{
		MidIndex = Start + (Count / 2);
	}

	return MidIndex;
}

FSplitInfo UBVHierarchy::FindBestSplit(const FAABB& CurrentBounds, TArray<FBin>& Bins, const FBuildContext& Context) const
{
	int Start = Context.Length.Start;
	int Count = Context.Length.Count;
	FVector CentroidExtent = Context.CentroidExtent;
	FVector CentroidMin = Context.CentroidMin;

	uint32 NumBins = Bins.size();

	FSplitInfo BestSplit{};
	for (uint32 Axis = 0; Axis < 3; Axis++)
	{
		if (CentroidExtent[Axis] < 1e-6f)
		{
			continue;
		}

		for (uint32 i = Start; i < Start + Count; i++)
		{
			float RelativePosition = (Primitives[i].Center[Axis] - CentroidMin[Axis]) / CentroidExtent[Axis];
			uint32 BinIndex = static_cast<uint32>(RelativePosition * NumBins);
			BinIndex = std::min(BinIndex, NumBins - 1);
			Bins[BinIndex].PrimitiveCount++;
			Bins[BinIndex].Bounds = Bins[BinIndex].Bounds.Union(Bins[BinIndex].Bounds, Primitives[i].Bounds);
		}

		for (uint32 SplitPoint = 1; SplitPoint < NumBins; SplitPoint++)
		{
			FAABB LeftBounds =  FAABB::GetEmptyAABB();
			uint32 LeftCount = 0;
			for (uint32 i = 0; i < SplitPoint; i++)
			{
				LeftBounds = LeftBounds.Union(LeftBounds, Bins[i].Bounds);
				LeftCount += Bins[i].PrimitiveCount;
			}

			FAABB RightBounds =  FAABB::GetEmptyAABB();
			uint32 RightCount = 0;
			for (uint32 i = SplitPoint; i < NumBins; i++)
			{
				RightBounds = RightBounds.Union(RightBounds, Bins[i].Bounds);
				RightCount += Bins[i].PrimitiveCount;
			}

			if (LeftCount > 0 && RightCount > 0)
			{
				float ParentSurfaceArea = CurrentBounds.SurfaceArea();
				if (ParentSurfaceArea > 1.0e-6f)
				{
					float CurrentCost = 1.0f + (LeftBounds.SurfaceArea() * static_cast<float>(LeftCount) / ParentSurfaceArea)
									+ (RightBounds.SurfaceArea() * static_cast<float>(RightCount) / ParentSurfaceArea);

					if (CurrentCost < BestSplit.Cost)
					{
						BestSplit.Cost = CurrentCost;
						BestSplit.Axis = Axis;
						BestSplit.BinIndex = SplitPoint;
					}
				}
			}
		}
	}

	return BestSplit;
}
