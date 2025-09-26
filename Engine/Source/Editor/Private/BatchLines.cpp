#include "pch.h"
#include "Editor/Public/BatchLines.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/BVH/public/BVHManager.h"

UBatchLines::UBatchLines()
	: Grid()
	, BoundingBoxLines()
{
	Vertices.reserve(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());
	Vertices.resize(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());

	Grid.MergeVerticesAt(Vertices, 0);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());

	SetIndices();

	URenderer& Renderer = URenderer::GetInstance();

	//BatchLineConstData.CellSize = 1.0f;
	//BatchLineConstData.BoundingBoxModel = FMatrix::Identity();

	/*AddWorldGridVerticesAndConstData();
	AddBoundingBoxVertices();*/

	Primitive.NumVertices = static_cast<uint32>(Vertices.size());
	Primitive.NumIndices = static_cast<uint32>(Indices.size());
	Primitive.IndexBuffer = Renderer.CreateIndexBuffer(Indices.data(), Primitive.NumIndices * sizeof(uint32));
	//Primitive.Color = FVector4(1, 1, 1, 0.2f);
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(
		Vertices.data(), Primitive.NumVertices * sizeof(FVector), true);
	/*Primitive.Location = FVector(0, 0, 0);
	Primitive.Rotation = FVector(0, 0, 0);
	Primitive.Scale = FVector(1, 1, 1);*/
	Primitive.VertexShader = UAssetManager::GetInstance().GetVertexShader(EShaderType::BatchLine);
	Primitive.InputLayout = UAssetManager::GetInstance().GetIputLayout(EShaderType::BatchLine);
	Primitive.PixelShader = UAssetManager::GetInstance().GetPixelShader(EShaderType::BatchLine);
	VertexCapacity = Primitive.NumVertices;
	IndexCapacity = Primitive.NumIndices;
}

UBatchLines::~UBatchLines()
{
	URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
	Primitive.InputLayout->Release();
	Primitive.VertexShader->Release();
	URenderer::ReleaseIndexBuffer(Primitive.IndexBuffer);
	Primitive.PixelShader->Release();
}

void UBatchLines::UpdateUGridVertices(const float newCellSize)
{
	if (newCellSize == Grid.GetCellSize())
	{
		return;
	}
	Grid.UpdateVerticesBy(newCellSize);
	Grid.MergeVerticesAt(Vertices, 0);
	bChangedVertices = true;
}

void UBatchLines::UpdateBoundingBoxVertices(const FAABB& newBoundingBoxInfo)
{
	FAABB curBoudingBoxInfo = BoundingBoxLines.GetRenderedBoxInfo();
	if (newBoundingBoxInfo.Min == curBoudingBoxInfo.Min && newBoundingBoxInfo.Max == curBoudingBoxInfo.Max)
	{
		return;
	}

	BoundingBoxLines.UpdateVertices(newBoundingBoxInfo);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());
	bChangedVertices = true;
}

void UBatchLines::UpdateBatchLineVertices(const float newCellSize, const FAABB& newBoundingBoxInfo)
{
	UpdateUGridVertices(newCellSize);

	TArray<FAABB> CombinedBoxes;
	TArray<FAABB>& Boxes = UBVHManager::GetInstance().GetBoxes();
	CombinedBoxes.reserve(Boxes.size() + 1);

	// if (newBoundingBoxInfo.Min != newBoundingBoxInfo.Max)
	// {
	CombinedBoxes.push_back(newBoundingBoxInfo);
	// }

	CombinedBoxes.insert(CombinedBoxes.end(), Boxes.begin(), Boxes.end());
	SetBoundingBoxes(CombinedBoxes);
}

void UBatchLines::SetBoundingBoxes(const TArray<FAABB>& InBoxes)
{
	const uint32 gridVertexCount = Grid.GetNumVertices();

	Vertices.resize(gridVertexCount);
	Grid.MergeVerticesAt(Vertices, 0);
	Vertices.reserve(gridVertexCount + static_cast<uint32>(InBoxes.size()) * 8);

	Indices.clear();
	Indices.reserve(gridVertexCount + static_cast<uint32>(InBoxes.size()) * 24);
	for (uint32 index = 0; index < gridVertexCount; ++index)
	{
		Indices.push_back(index);
	}

	for (const FAABB& Box : InBoxes)
	{
		const uint32 baseVertex = static_cast<uint32>(Vertices.size());

		Vertices.push_back({ Box.Min.X, Box.Min.Y, Box.Min.Z });
		Vertices.push_back({ Box.Max.X, Box.Min.Y, Box.Min.Z });
		Vertices.push_back({ Box.Max.X, Box.Max.Y, Box.Min.Z });
		Vertices.push_back({ Box.Min.X, Box.Max.Y, Box.Min.Z });
		Vertices.push_back({ Box.Min.X, Box.Min.Y, Box.Max.Z });
		Vertices.push_back({ Box.Max.X, Box.Min.Y, Box.Max.Z });
		Vertices.push_back({ Box.Max.X, Box.Max.Y, Box.Max.Z });
		Vertices.push_back({ Box.Min.X, Box.Max.Y, Box.Max.Z });

		static constexpr uint32 BoxIndices[24] = {
			0, 1, 1, 2, 2, 3, 3, 0,
			4, 5, 5, 6, 6, 7, 7, 4,
			0, 4, 1, 5, 2, 6, 3, 7
		};

		for (uint32 idx : BoxIndices)
		{
			Indices.push_back(baseVertex + idx);
		}
	}

	bChangedVertices = true;
	const uint32 newIndexCount = static_cast<uint32>(Indices.size());
	bChangedIndices = (Primitive.IndexBuffer == nullptr) || (newIndexCount != IndexCapacity);
	Primitive.NumVertices = static_cast<uint32>(Vertices.size());
	Primitive.NumIndices = newIndexCount;
}

void UBatchLines::UpdateVertexBuffer()
{
	URenderer& Renderer = URenderer::GetInstance();

	const uint32 vertexCount = static_cast<uint32>(Vertices.size());
	if (bChangedVertices)
	{
		if (vertexCount == 0)
		{
			URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
			Primitive.Vertexbuffer = nullptr;
			VertexCapacity = 0;
		}
		else if (!Primitive.Vertexbuffer || vertexCount > VertexCapacity)
		{
			URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
			Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(Vertices.data(), vertexCount * sizeof(FVector), true);
			VertexCapacity = vertexCount;
		}
		else
		{
			Renderer.UpdateVertexBuffer(Primitive.Vertexbuffer, Vertices);
		}
	}

	const uint32 indexCount = static_cast<uint32>(Indices.size());
	if (bChangedIndices)
	{
		URenderer::ReleaseIndexBuffer(Primitive.IndexBuffer);
		if (indexCount > 0)
		{
			Primitive.IndexBuffer = Renderer.CreateIndexBuffer(Indices.data(), indexCount * sizeof(uint32));
		}
		else
		{
			Primitive.IndexBuffer = nullptr;
		}
		IndexCapacity = indexCount;
	}

	Primitive.NumVertices = vertexCount;
	Primitive.NumIndices = indexCount;

	bChangedVertices = false;
	bChangedIndices = false;
}

void UBatchLines::Render()
{
	URenderer& Renderer = URenderer::GetInstance();

	// to do: 아래 함수를 batch에 맞게 수정해야 함.
	Renderer.RenderPrimitiveIndexed(Primitive, Primitive.RenderState, false, sizeof(FVector), sizeof(uint32));
}

void UBatchLines::SetIndices()
{
	Indices.clear();
	const uint32 numGridVertices = Grid.GetNumVertices();
	Indices.reserve(numGridVertices + 24);

	for (uint32 index = 0; index < numGridVertices; ++index)
	{
		Indices.push_back(index);
	}

	uint32 boundingBoxLineIdx[] = {
		// 앞면
		0, 1,
		1, 2,
		2, 3,
		3, 0,

		// 뒷면
		4, 5,
		5, 6,
		6, 7,
		7, 4,

		// 옆면 연결
		0, 4,
		1, 5,
		2, 6,
		3, 7
	};

	for (uint32 i = 0; i < std::size(boundingBoxLineIdx); ++i)
	{
		Indices.push_back(numGridVertices + boundingBoxLineIdx[i]);
	}
}

