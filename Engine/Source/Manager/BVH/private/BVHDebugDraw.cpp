#include "pch.h"
#include "Manager/BVH/public/BVHDebugDraw.h"

#include "Editor/Public/BatchLines.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Render/Renderer/Public/Renderer.h"

namespace
{
	constexpr uint32 EDGES_PER_BOX = 24; // 12 line segments * 2 indices

	void AppendBoxGeometry(const FAABB& Box, TArray<FVector>& OutVertices, TArray<uint32>& OutIndices)
	{
		const uint32 BaseIndex = static_cast<uint32>(OutVertices.size());
		const FVector& Min = Box.Min;
		const FVector& Max = Box.Max;

		OutVertices.push_back({ Min.X, Min.Y, Min.Z });
		OutVertices.push_back({ Max.X, Min.Y, Min.Z });
		OutVertices.push_back({ Max.X, Max.Y, Min.Z });
		OutVertices.push_back({ Min.X, Max.Y, Min.Z });
		OutVertices.push_back({ Min.X, Min.Y, Max.Z });
		OutVertices.push_back({ Max.X, Min.Y, Max.Z });
		OutVertices.push_back({ Max.X, Max.Y, Max.Z });
		OutVertices.push_back({ Min.X, Max.Y, Max.Z });

		static constexpr uint32 BoxIndices[EDGES_PER_BOX] =
		{
			0, 1,
			1, 2,
			2, 3,
			3, 0,

			4, 5,
			5, 6,
			6, 7,
			7, 4,

			0, 4,
			1, 5,
			2, 6,
			3, 7
		};

		for (uint32 Index : BoxIndices)
		{
			OutIndices.push_back(BaseIndex + Index);
		}
	}
}

UBVHDebugDraw::UBVHDebugDraw()
{
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	// Primitive.RenderState.CullMode = ECullMode::None;
	Primitive.RenderState.FillMode = EFillMode::WireFrame;
	Primitive.Color = FVector4(1.f, 0.6f, 0.f, 1.f);
	Primitive.Location = FVector(0.f, 0.f, 0.f);
	Primitive.Rotation = FVector(0.f, 0.f, 0.f);
	Primitive.Scale = FVector(1.f, 1.f, 1.f);
	Primitive.bShouldAlwaysVisible = true;

	auto& AssetManager = UAssetManager::GetInstance();
	Primitive.VertexShader = AssetManager.GetVertexShader(EShaderType::BatchLine);
	Primitive.PixelShader = AssetManager.GetPixelShader(EShaderType::BatchLine);
	Primitive.InputLayout = AssetManager.GetIputLayout(EShaderType::BatchLine);
}

UBVHDebugDraw::~UBVHDebugDraw()
{
	ReleaseBuffers();

	if (Primitive.InputLayout)
	{
		Primitive.InputLayout->Release();
		Primitive.InputLayout = nullptr;
	}

	if (Primitive.VertexShader)
	{
		Primitive.VertexShader->Release();
		Primitive.VertexShader = nullptr;
	}

	if (Primitive.PixelShader)
	{
		Primitive.PixelShader->Release();
		Primitive.PixelShader = nullptr;
	}
}

void UBVHDebugDraw::SetBoxes(const TArray<FAABB>& InBoxes)
{
	if (InBoxes.empty())
	{
		Clear();
		return;
	}
}

void UBVHDebugDraw::Clear()
{
	bVisible = true;
	Primitive.NumVertices = 0;
	Primitive.NumIndices = 0;
	Vertices.clear();
	Indices.clear();
}

void UBVHDebugDraw::Render(const TArray<FAABB>& InBoxes) const
{
	if (!bVisible || Primitive.NumIndices == 0 || !Primitive.Vertexbuffer || !Primitive.IndexBuffer)
	{
		return;
	}
}

void UBVHDebugDraw::ReleaseBuffers()
{
	if (Primitive.Vertexbuffer)
	{
		URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
		Primitive.Vertexbuffer = nullptr;
	}

	if (Primitive.IndexBuffer)
	{
		URenderer::ReleaseIndexBuffer(Primitive.IndexBuffer);
		Primitive.IndexBuffer = nullptr;
	}

	AllocatedVertexCapacity = 0;
	AllocatedIndexCapacity = 0;
}
