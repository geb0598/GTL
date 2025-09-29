#include "pch.h" // 프로젝트의 Precompiled Header
#include "Source/Component/Mesh/Public/StaticMesh.h" // UStaticMesh 클래스 자신의 헤더

#include <algorithm>
#include <functional>

#include "Physics/Public/RayIntersection.h"

// 클래스 구현 매크로
IMPLEMENT_CLASS(UStaticMesh, UObject)

UStaticMesh::UStaticMesh()
	: StaticMeshAsset(nullptr) // 멤버 변수를 nullptr로 명시적 초기화
{
}

UStaticMesh::~UStaticMesh()
{
	// 이 클래스는 FStaticMesh 데이터의 소유자가 아닙니다.
	// 실제 데이터는 AssetManager가 관리하므로, 여기서는 아무것도 하지 않습니다.

	// 임시로 할당된 Material 해제 -> 이후 GUObject에서 관리 예정
	for (UMaterial* Material : Materials)
	{
		SafeDelete(Material);
	}
	Materials.clear();
}

void UStaticMesh::SetStaticMeshAsset(FStaticMesh* InStaticMeshAsset)
{
	StaticMeshAsset = InStaticMeshAsset;

	if (StaticMeshAsset)
	{
		StaticMeshAsset->bTriangleBVHDirty = true;
		StaticMeshAsset->TriangleBVHNodes.clear();
		StaticMeshAsset->TriangleBVHPrimitives.clear();
		StaticMeshAsset->TriangleBVHRoot = -1;
	}

	EnsureTriangleBVH();
}

const FName& UStaticMesh::GetAssetPathFileName() const
{
	// 항상 포인터가 유효한지 확인 후 접근해야 합니다.
	if (StaticMeshAsset)
	{
		return StaticMeshAsset->PathFileName;
	}

	// 포인터가 null일 경우, 안전하게 비어있는 static 객체를 반환합니다.
	static const FName EmptyString = "";
	return EmptyString;
}

const TArray<FNormalVertex>& UStaticMesh::GetVertices() const
{
	if (StaticMeshAsset)
	{
		return StaticMeshAsset->Vertices;
	}
	static const TArray<FNormalVertex> EmptyVertices;
	return EmptyVertices;
}

TArray<FNormalVertex>& UStaticMesh::GetVertices()
{
	if (StaticMeshAsset)
	{
		return StaticMeshAsset->Vertices;
	}
	static TArray<FNormalVertex> EmptyVertices;
	return EmptyVertices;
}

const TArray<uint32>& UStaticMesh::GetIndices() const
{
	if (StaticMeshAsset)
	{
		return StaticMeshAsset->Indices;
	}
	static const TArray<uint32> EmptyIndices;
	return EmptyIndices;
}

UMaterial* UStaticMesh::GetMaterial(int32 MaterialIndex) const
{
	return (MaterialIndex >= 0 && MaterialIndex < Materials.size()) ? Materials[MaterialIndex] : nullptr;
}

void UStaticMesh::SetMaterial(int32 MaterialIndex, UMaterial* Material)
{
	if (MaterialIndex >= 0)
	{
		// 배열 크기가 부족하면 확장
		if (MaterialIndex >= static_cast<int32>(Materials.size()))
		{
			Materials.resize(MaterialIndex + 1, nullptr);
		}
		Materials[MaterialIndex] = Material;
	}
}

int32 UStaticMesh::GetNumMaterials() const
{
	return Materials.size();
}

const TArray<FMeshSection>& UStaticMesh::GetSections() const
{
	if (StaticMeshAsset)
	{
		return StaticMeshAsset->Sections;
	}
	static const TArray<FMeshSection> EmptySections;
	return EmptySections;
}

inline float SurfaceArea(const FAABB& b)
{
    const FVector d = b.Max - b.Min;
    // Guard: negative/degenerate boxes
    if (d.X <= 0.f || d.Y <= 0.f || d.Z <= 0.f) return 0.f;
    return 2.0f * (d.X*d.Y + d.Y*d.Z + d.Z*d.X);
}

inline void ExpandToFit(FAABB& dst, const FAABB& src)
{
    dst.Min.X = std::min(dst.Min.X, src.Min.X);
    dst.Min.Y = std::min(dst.Min.Y, src.Min.Y);
    dst.Min.Z = std::min(dst.Min.Z, src.Min.Z);
    dst.Max.X = std::max(dst.Max.X, src.Max.X);
    dst.Max.Y = std::max(dst.Max.Y, src.Max.Y);
    dst.Max.Z = std::max(dst.Max.Z, src.Max.Z);
}

inline void ExpandToFitPoint(FAABB& dst, const FVector& p)
{
    dst.Min.X = std::min(dst.Min.X, p.X);
    dst.Min.Y = std::min(dst.Min.Y, p.Y);
    dst.Min.Z = std::min(dst.Min.Z, p.Z);
    dst.Max.X = std::max(dst.Max.X, p.X);
    dst.Max.Y = std::max(dst.Max.Y, p.Y);
    dst.Max.Z = std::max(dst.Max.Z, p.Z);
}

// Safe division
inline float SafeRcp(float x) { return (fabsf(x) > 1e-20f) ? (1.0f / x) : 0.0f; }

template<int NBINS>
struct FBin
{
    FAABB b;   // bounds of all prims in this bin
    int   n;   // count
    FBin()
    : b(FAABB(FVector(FLT_MAX,FLT_MAX,FLT_MAX), FVector(-FLT_MAX,-FLT_MAX,-FLT_MAX)))
    , n(0) {}
};

void UStaticMesh::EnsureTriangleBVH() const
{
	if (!StaticMeshAsset)
	{
		return;
	}

	if (!StaticMeshAsset->bTriangleBVHDirty &&
		StaticMeshAsset->TriangleBVHRoot >= 0 &&
		!StaticMeshAsset->TriangleBVHNodes.empty())
	{
		return;
	}

	StaticMeshAsset->TriangleBVHNodes.clear();
	StaticMeshAsset->TriangleBVHPrimitives.clear();
	StaticMeshAsset->TriangleBVHRoot = -1;

	const TArray<FNormalVertex>& Vertices = StaticMeshAsset->Vertices;
	const TArray<uint32>& Indices = StaticMeshAsset->Indices;

	const bool bHasIndices = !Indices.empty();
	const size_t TriangleCount = bHasIndices ? (Indices.size() / 3) : (Vertices.size() / 3);
	if (TriangleCount == 0)
	{
		StaticMeshAsset->bTriangleBVHDirty = false;
		return;
	}

	StaticMeshAsset->TriangleBVHPrimitives.reserve(TriangleCount);

	auto AddPrimitive = [&](uint32 i0, uint32 i1, uint32 i2)
	{
		const FVector& P0 = Vertices[i0].Position;
		const FVector& P1 = Vertices[i1].Position;
		const FVector& P2 = Vertices[i2].Position;

		FVector minP( std::min({P0.X, P1.X, P2.X}),
					  std::min({P0.Y, P1.Y, P2.Y}),
					  std::min({P0.Z, P1.Z, P2.Z}) );
		FVector maxP( std::max({P0.X, P1.X, P2.X}),
					  std::max({P0.Y, P1.Y, P2.Y}),
					  std::max({P0.Z, P1.Z, P2.Z}) );

		FTriangleBVHPrimitive prim;
		prim.Bounds = FAABB(minP, maxP);
		prim.Center = (P0 + P1 + P2) / 3.0f;

		prim.Indices[0] = i0;
		prim.Indices[1] = i1;
		prim.Indices[2] = i2;

		prim.V0 = P0;
		prim.E0 = P1 - P0;
		prim.E1 = P2 - P0;

		StaticMeshAsset->TriangleBVHPrimitives.push_back(prim);
	};

	if (bHasIndices)
	{
		for (size_t i = 0; i + 2 < Indices.size(); i += 3)
		{
			AddPrimitive(Indices[i + 0], Indices[i + 1], Indices[i + 2]);
		}
	}
	else
	{
		for (size_t i = 0; i + 2 < Vertices.size(); i += 3)
		{
			AddPrimitive(static_cast<uint32>(i + 0), static_cast<uint32>(i + 1), static_cast<uint32>(i + 2));
		}
	}

	if (StaticMeshAsset->TriangleBVHPrimitives.empty())
	{
		StaticMeshAsset->bTriangleBVHDirty = false;
		return;
	}

	constexpr int32 MaxLeafSize = 8;
	static constexpr int NBINS = 16;        // 8/16/32 are common; 16 is a good default
	static constexpr float Ct = 1.0f;       // traversal cost
	static constexpr float Ci = 1.0f;       // primitive intersection cost

	std::function<int32(int32,int32)> BuildBVH;
	BuildBVH = [&](int32 Start, int32 Count) -> int32
	{
	    // 1) Compute node bounds and centroid bounds
	    FAABB nodeBounds(FVector(FLT_MAX,FLT_MAX,FLT_MAX), FVector(-FLT_MAX,-FLT_MAX,-FLT_MAX));
	    FAABB centroidBounds(FVector(FLT_MAX,FLT_MAX,FLT_MAX), FVector(-FLT_MAX,-FLT_MAX,-FLT_MAX));

	    for (int32 i = 0; i < Count; ++i)
	    {
	        const FTriangleBVHPrimitive& p = StaticMeshAsset->TriangleBVHPrimitives[Start + i];
	        ExpandToFit(nodeBounds, p.Bounds);
	        ExpandToFitPoint(centroidBounds, p.Center);
	    }

	    // Leaf test (small or degenerate)
	    const FVector cbSize = centroidBounds.Max - centroidBounds.Min;
	    const bool degenerateAxis = (cbSize.X <= 1e-8f) && (cbSize.Y <= 1e-8f) && (cbSize.Z <= 1e-8f);
	    const float leafCost = Ci * float(Count);
	    if (Count <= MaxLeafSize || degenerateAxis)
	    {
	        FTriangleBVHNode node;
	        node.Bounds   = nodeBounds;
	        node.bIsLeaf  = true;
	        node.Start    = Start;
	        node.Count    = Count;
	        node.LeftChild = node.RightChild = -1;

	        const int32 nodeIdx = (int32)StaticMeshAsset->TriangleBVHNodes.size();
	        StaticMeshAsset->TriangleBVHNodes.push_back(node);
	        return nodeIdx;
	    }

	    // 2) Choose axis with largest centroid extent
	    int axis = 0;
	    if (cbSize.Y > cbSize.X) axis = 1;
	    if (cbSize.Z > (axis==0 ? cbSize.X : cbSize.Y)) axis = 2;

	    // 3) Bin the primitives along 'axis'
	    FBin<NBINS> bins[NBINS];
	    const float cmin = (axis==0? centroidBounds.Min.X : (axis==1? centroidBounds.Min.Y : centroidBounds.Min.Z));
	    const float extent = (axis==0? cbSize.X : (axis==1? cbSize.Y : cbSize.Z));
	    const float invExtent = SafeRcp(extent); // if zero, we would have leaf-ed above

	    auto binIndexOf = [&](const FVector& c)->int {
	        const float coord = (axis==0? c.X : (axis==1? c.Y : c.Z));
	        float u = (coord - cmin) * invExtent;   // [0,1]
	        int idx = (int)(u * NBINS);
	        if (idx < 0) idx = 0;
	        if (idx >= NBINS) idx = NBINS - 1;
	        return idx;
	    };

	    for (int32 i = 0; i < Count; ++i)
	    {
	        const FTriangleBVHPrimitive& p = StaticMeshAsset->TriangleBVHPrimitives[Start + i];
	        const int bi = binIndexOf(p.Center);
	        ExpandToFit(bins[bi].b, p.Bounds);
	        bins[bi].n++;
	    }

	    // 4) Prefix scan (left) and suffix scan (right) of bins to evaluate SAH cost per split
	    FAABB leftB[NBINS], rightB[NBINS];
	    int   leftN[NBINS], rightN[NBINS];

	    // Left-to-right
	    {
	        FAABB accB(FVector(FLT_MAX,FLT_MAX,FLT_MAX), FVector(-FLT_MAX,-FLT_MAX,-FLT_MAX));
	        int accN = 0;
	        for (int i = 0; i < NBINS; ++i)
	        {
	            if (bins[i].n > 0) ExpandToFit(accB, bins[i].b);
	            accN += bins[i].n;
	            leftB[i] = accB;
	            leftN[i] = accN;
	        }
	    }

	    // Right-to-left
	    {
	        FAABB accB(FVector(FLT_MAX,FLT_MAX,FLT_MAX), FVector(-FLT_MAX,-FLT_MAX,-FLT_MAX));
	        int accN = 0;
	        for (int i = NBINS-1; i >= 0; --i)
	        {
	            if (bins[i].n > 0) ExpandToFit(accB, bins[i].b);
	            accN += bins[i].n;
	            rightB[i] = accB;
	            rightN[i] = accN;
	        }
	    }

	    const float invParentSA = SafeRcp(std::max(SurfaceArea(nodeBounds), 1e-20f));

	    // Try splits between bins: split after bin s (left uses [0..s], right uses [s+1..NBINS-1])
	    float bestCost = FLT_MAX;
	    int   bestSplitBin = -1;

	    for (int s = 0; s < NBINS-1; ++s)
	    {
	        const int nL = leftN[s];
	        const int nR = rightN[s+1];
	        if (nL == 0 || nR == 0) continue; // avoid empty side

	        const float sah =
	            Ct +
	            (SurfaceArea(leftB[s])  * invParentSA) * (Ci * float(nL)) +
	            (SurfaceArea(rightB[s+1]) * invParentSA) * (Ci * float(nR));

	        if (sah < bestCost)
	        {
	            bestCost = sah;
	            bestSplitBin = s;
	        }
	    }

	    // 5) SAH decision: if splitting isn’t better than a leaf, make a leaf
	    if (bestSplitBin == -1 || bestCost >= leafCost)
	    {
	        FTriangleBVHNode node;
	        node.Bounds   = nodeBounds;
	        node.bIsLeaf  = true;
	        node.Start    = Start;
	        node.Count    = Count;
	        node.LeftChild = node.RightChild = -1;

	        const int32 nodeIdx = (int32)StaticMeshAsset->TriangleBVHNodes.size();
	        StaticMeshAsset->TriangleBVHNodes.push_back(node);
	        return nodeIdx;
	    }

	    // 6) Partition primitives in-place by the chosen split
	    auto beginIt = StaticMeshAsset->TriangleBVHPrimitives.begin() + Start;
	    auto endIt   = beginIt + Count;

	    const int splitBin = bestSplitBin;
	    auto midIt = std::stable_partition(beginIt, endIt, [&](const FTriangleBVHPrimitive& p)
	    {
	        const int bi = binIndexOf(p.Center);
	        return bi <= splitBin;
	    });

	    int32 leftCount = int32(midIt - beginIt);
	    int32 rightCount = Count - leftCount;

	    // Degenerate guard: if one side is empty (can happen with heavy clustering),
	    // fall back to median split to guarantee progress.
	    if (leftCount == 0 || rightCount == 0)
	    {
	        // median by centroid along chosen axis
	        const int32 mid = Start + Count/2;
	        auto key = [axis](const FTriangleBVHPrimitive& A, const FTriangleBVHPrimitive& B)
	        {
	            const float a = (axis==0? A.Center.X : (axis==1? A.Center.Y : A.Center.Z));
	            const float b = (axis==0? B.Center.X : (axis==1? B.Center.Y : B.Center.Z));
	            return a < b;
	        };
	        std::nth_element(beginIt, beginIt + (mid - Start), endIt, key);
	        leftCount  = mid - Start;
	        rightCount = Count - leftCount;
	        midIt      = beginIt + leftCount;
	    }

	    // 7) Create inner node and recurse
	    const int32 leftChild  = BuildBVH(Start, leftCount);
	    const int32 rightChild = BuildBVH(Start + leftCount, rightCount);

	    FTriangleBVHNode node;
	    node.Bounds     = nodeBounds;
	    node.bIsLeaf    = false;
	    node.Start      = Start;
	    node.Count      = Count;
	    node.LeftChild  = leftChild;
	    node.RightChild = rightChild;

	    const int32 nodeIdx = (int32)StaticMeshAsset->TriangleBVHNodes.size();
	    StaticMeshAsset->TriangleBVHNodes.push_back(node);
	    return nodeIdx;
	};

	StaticMeshAsset->TriangleBVHRoot = BuildBVH(0, static_cast<int32>(StaticMeshAsset->TriangleBVHPrimitives.size()));
	StaticMeshAsset->bTriangleBVHDirty = false;
}

bool UStaticMesh::RaycastTriangleBVH(const FRay& ModelRay, float& InOutDistance) const
{
	if (!StaticMeshAsset) return false;
    const int32 root = StaticMeshAsset->TriangleBVHRoot;
    if (root < 0 || StaticMeshAsset->TriangleBVHNodes.empty()) return false;

    // Cache local refs/pointers (avoids repeated indirections)
    const auto& Nodes = StaticMeshAsset->TriangleBVHNodes;
    const auto& Prims = StaticMeshAsset->TriangleBVHPrimitives;

    // Wider stack + reuse if you want: thread_local keeps a single buffer per thread.
    static thread_local int32 Stack[128];
    int32 sp = 0;
    Stack[sp++] = root;

    bool  hit = false;
    float closest = InOutDistance;

    while (sp > 0)
    {
        const int32 ni = Stack[--sp];
        const FTriangleBVHNode& node = Nodes[ni];

        float tEntry;
        if (!node.Bounds.RaycastHit(ModelRay, &tEntry) || tEntry > closest)
            continue;

        if (node.bIsLeaf)
        {
            // Tight loop; keep everything in registers
            for (int32 i = 0; i < node.Count; ++i)
            {
                const FTriangleBVHPrimitive& p = Prims[node.Start + i];

                // Optional: fast AABB pre-test against triangle bounds to skip obviously far triangles
                float tTriBoxEntry;
                if (!p.Bounds.RaycastHit(ModelRay, &tTriBoxEntry) || tTriBoxEntry > closest)
                    continue;

                float t;
                if (RayHitTriangle_MT(ModelRay, p.V0, p.E0, p.E1, closest, t))
                {
                    closest = t;
                    hit = true;
                    // Don’t early-exit here: a later triangle in the same leaf might be even closer.
                }
            }
        }
        else
        {
            // Compute near/far by AABB entry distance
            float tL = FLT_MAX, tR = FLT_MAX;
            const bool hasL = (node.LeftChild  != -1) && Nodes[node.LeftChild ].Bounds.RaycastHit(ModelRay, &tL) && tL <= closest;
            const bool hasR = (node.RightChild != -1) && Nodes[node.RightChild].Bounds.RaycastHit(ModelRay, &tR) && tR <= closest;

            if (hasL && hasR)
            {
                // Push far first so near is popped first (LIFO stack)
                if (tL < tR)
                {
                    if (sp < 128) Stack[sp++] = node.RightChild;
                    if (sp < 128) Stack[sp++] = node.LeftChild;
                }
                else
                {
                    if (sp < 128) Stack[sp++] = node.LeftChild;
                    if (sp < 128) Stack[sp++] = node.RightChild;
                }
            }
            else if (hasL)
            {
                if (sp < 128) Stack[sp++] = node.LeftChild;
            }
            else if (hasR)
            {
                if (sp < 128) Stack[sp++] = node.RightChild;
            }

            // If neither child intersects within 'closest', we naturally prune.
        }
    }

    if (hit) InOutDistance = closest;
    return hit;
}

// LOD System Implementation
void UStaticMesh::AddLODMesh(FStaticMesh* LODMesh)
{
	if (LODMesh)
	{
		LODMeshes.push_back(LODMesh);
	}
}

FStaticMesh* UStaticMesh::GetLODMesh(int32 LODLevel) const
{
	if (LODLevel >= 0 && LODLevel < static_cast<int32>(LODMeshes.size()))
	{
		return LODMeshes[LODLevel];
	}
	return StaticMeshAsset; // LOD가 없으면 원본 반환
}
