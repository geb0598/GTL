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

	auto AddPrimitive = [&](uint32 Index0, uint32 Index1, uint32 Index2)
	{
		const FVector& V0 = Vertices[Index0].Position;
		const FVector& V1 = Vertices[Index1].Position;
		const FVector& V2 = Vertices[Index2].Position;

		FVector MinPoint(
			std::min(std::min(V0.X, V1.X), V2.X),
			std::min(std::min(V0.Y, V1.Y), V2.Y),
			std::min(std::min(V0.Z, V1.Z), V2.Z));

		FVector MaxPoint(
			std::max(std::max(V0.X, V1.X), V2.X),
			std::max(std::max(V0.Y, V1.Y), V2.Y),
			std::max(std::max(V0.Z, V1.Z), V2.Z));

		FTriangleBVHPrimitive Primitive;
		Primitive.Bounds = FAABB(MinPoint, MaxPoint);
		Primitive.Center = FVector(
			(V0.X + V1.X + V2.X) / 3.0f,
			(V0.Y + V1.Y + V2.Y) / 3.0f,
			(V0.Z + V1.Z + V2.Z) / 3.0f);
		Primitive.Indices[0] = Index0;
		Primitive.Indices[1] = Index1;
		Primitive.Indices[2] = Index2;

		StaticMeshAsset->TriangleBVHPrimitives.push_back(Primitive);
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

	std::function<int32(int32, int32)> BuildBVH;
	BuildBVH = [&](int32 Start, int32 Count) -> int32
	{
		FTriangleBVHNode Node;
		Node.Bounds = StaticMeshAsset->TriangleBVHPrimitives[Start].Bounds;
		for (int32 i = 1; i < Count; ++i)
		{
			const FAABB& Other = StaticMeshAsset->TriangleBVHPrimitives[Start + i].Bounds;
			Node.Bounds = Node.Bounds.Union(Node.Bounds, Other);
		}

		if (Count <= MaxLeafSize)
		{
			Node.bIsLeaf = true;
			Node.Start = Start;
			Node.Count = Count;

			const int32 NodeIndex = static_cast<int32>(StaticMeshAsset->TriangleBVHNodes.size());
			StaticMeshAsset->TriangleBVHNodes.push_back(Node);
			return NodeIndex;
		}

		FVector Mean(0.0f, 0.0f, 0.0f);
		for (int32 i = 0; i < Count; ++i)
		{
			Mean += StaticMeshAsset->TriangleBVHPrimitives[Start + i].Center;
		}
		Mean /= static_cast<float>(Count);

		FVector Variance(0.0f, 0.0f, 0.0f);
		for (int32 i = 0; i < Count; ++i)
		{
			FVector Delta = StaticMeshAsset->TriangleBVHPrimitives[Start + i].Center - Mean;
			Variance.X += Delta.X * Delta.X;
			Variance.Y += Delta.Y * Delta.Y;
			Variance.Z += Delta.Z * Delta.Z;
		}

		int32 Axis = 0;
		if (Variance.Y > Variance.X) { Axis = 1; }
		if (Variance.Z > Variance[Axis]) { Axis = 2; }

		const int32 Mid = Start + Count / 2;
		auto Begin = StaticMeshAsset->TriangleBVHPrimitives.begin() + Start;
		auto MidIt = StaticMeshAsset->TriangleBVHPrimitives.begin() + Mid;
		auto End = StaticMeshAsset->TriangleBVHPrimitives.begin() + Start + Count;
		std::nth_element(Begin, MidIt, End, [Axis](const FTriangleBVHPrimitive& A, const FTriangleBVHPrimitive& B)
		{
			return A.Center[Axis] < B.Center[Axis];
		});

		Node.LeftChild = BuildBVH(Start, Mid - Start);
		Node.RightChild = BuildBVH(Mid, Count - (Mid - Start));

		const int32 NodeIndex = static_cast<int32>(StaticMeshAsset->TriangleBVHNodes.size());
		StaticMeshAsset->TriangleBVHNodes.push_back(Node);
		return NodeIndex;
	};

	StaticMeshAsset->TriangleBVHRoot = BuildBVH(0, static_cast<int32>(StaticMeshAsset->TriangleBVHPrimitives.size()));
	StaticMeshAsset->bTriangleBVHDirty = false;
}

bool UStaticMesh::RaycastTriangleBVH(const FRay& ModelRay, float& InOutDistance) const
{
	if (!StaticMeshAsset)
	{
		return false;
	}

	// EnsureTriangleBVH();

	if (StaticMeshAsset->TriangleBVHRoot < 0 || StaticMeshAsset->TriangleBVHNodes.empty())
	{
		return false;
	}

	constexpr int32 MaxStackDepth = 64;
	int32 Stack[MaxStackDepth];
	int32 StackSize = 0;
	Stack[StackSize++] = StaticMeshAsset->TriangleBVHRoot;

	const TArray<FTriangleBVHNode>& Nodes = StaticMeshAsset->TriangleBVHNodes;
	const TArray<FTriangleBVHPrimitive>& Primitives = StaticMeshAsset->TriangleBVHPrimitives;
	const TArray<FNormalVertex>& Vertices = StaticMeshAsset->Vertices;

	bool bHit = false;
	float Closest = InOutDistance;

	while (StackSize > 0)
	{
		const int32 NodeIndex = Stack[--StackSize];
		const FTriangleBVHNode& Node = Nodes[NodeIndex];

		float EntryDistance = 0.0f;
		if (!Node.Bounds.RaycastHit(ModelRay, &EntryDistance) || EntryDistance > Closest)
		{
			continue;
		}

		if (Node.bIsLeaf)
		{
			for (int32 i = 0; i < Node.Count; ++i)
			{
				const FTriangleBVHPrimitive& Primitive = Primitives[Node.Start + i];
				const FVector& V0 = Vertices[Primitive.Indices[0]].Position;
				const FVector& V1 = Vertices[Primitive.Indices[1]].Position;
				const FVector& V2 = Vertices[Primitive.Indices[2]].Position;

				float Distance = 0.0f;
				if (RayTriangleIntersectModel(ModelRay, V0, V1, V2, Distance) && Distance < Closest)
				{
					Closest = Distance;
					bHit = true;
				}
			}
		}
		else
		{
			if (Node.LeftChild != -1 && StackSize < MaxStackDepth)
			{
				Stack[StackSize++] = Node.LeftChild;
			}
			if (Node.RightChild != -1 && StackSize < MaxStackDepth)
			{
				Stack[StackSize++] = Node.RightChild;
			}
		}
	}

	if (bHit)
	{
		InOutDistance = Closest;
	}

	return bHit;
}
