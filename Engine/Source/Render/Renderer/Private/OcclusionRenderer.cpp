#include "pch.h"

#include "cpp-thread-pool/thread_pool.h"

#include "Global/CoreTypes.h"
#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Render/Renderer/Public/OcclusionRenderer.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/Camera.h"

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")

IMPLEMENT_SINGLETON_CLASS_BASE(UOcclusionRenderer)

UOcclusionRenderer::UOcclusionRenderer() = default;
UOcclusionRenderer::~UOcclusionRenderer()
{
	Release();
}

void UOcclusionRenderer::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, uint32 InWidth, uint32 InHeight)
{
	if (!InDevice)
	{
		return;
	}
	Device = InDevice;

	if (!InDeviceContext)
	{
		return;
	}
	DeviceContext = InDeviceContext;

	Width = InWidth;
	Height = InHeight;

	CreateShader(Device);
	CreateDepthResource(Device);
	CreateHiZResource(Device);
	CreateBoundingVolumeResource(Device);
	CreateVisibilityResource(Device);
}

void UOcclusionRenderer::Release()
{
	ReleaseShader();
	ReleaseDepthResource();
	ReleaseHiZResource();
	ReleaseBoundingVolumeResource();
	ReleaseVisibilityResource();

	Device = nullptr;
	DeviceContext = nullptr;
}

void UOcclusionRenderer::BuildScreenSpaceBoundingVolumes(ID3D11DeviceContext* InDeviceContext, UCamera* InCamera, const TArray<TObjectPtr<UPrimitiveComponent>>& PrimitiveComponents)
{
	// Clear previous bounding volumes
	BoundingVolumes.clear();
	BoundingVolumes.resize(PrimitiveComponents.size());

	if (PrimitiveComponents.empty())
	{
		return;
	}

	FViewProjConstants ViewProjConstants = InCamera->GetFViewProjConstants();
	FMatrix ViewProjectionMatrix = ViewProjConstants.View * ViewProjConstants.Projection;

	for (size_t i = 0; i < PrimitiveComponents.size(); ++i)
	{
		const auto& Primitive = PrimitiveComponents[i];
		if (!Primitive)
		{
			BoundingVolumes[i] = { FVector(0, 0, 0), FVector(0, 0, 0) };
			continue;
		}

		FVector WorldMin, WorldMax;
		Primitive->GetWorldAABB(WorldMin, WorldMax);

		// Get the 8 corners of the world-space AABB
		FVector corners[8] = {
			FVector(WorldMin.X, WorldMin.Y, WorldMin.Z),
			FVector(WorldMax.X, WorldMin.Y, WorldMin.Z),
			FVector(WorldMin.X, WorldMax.Y, WorldMin.Z),
			FVector(WorldMin.X, WorldMin.Y, WorldMax.Z),
			FVector(WorldMax.X, WorldMax.Y, WorldMin.Z),
			FVector(WorldMax.X, WorldMin.Y, WorldMax.Z),
			FVector(WorldMin.X, WorldMax.Y, WorldMax.Z),
			FVector(WorldMax.X, WorldMax.Y, WorldMax.Z)
		};

		FVector ClipMin = FVector(FLT_MAX, FLT_MAX, FLT_MAX);
		FVector ClipMax = FVector(FLT_MIN, FLT_MIN, FLT_MIN);

		for (int j = 0; j < 8; ++j)
		{
			FVector4 clipPos = FMatrix::VectorMultiply(FVector4(corners[j].X, corners[j].Y, corners[j].Z, 1.0f), ViewProjectionMatrix);

			// Perform perspective divide
			if (clipPos.W == 0.0f) continue; // Avoid division by zero

			FVector4 ndcPos;
			ndcPos.X = clipPos.X / clipPos.W;
			ndcPos.Y = clipPos.Y / clipPos.W;
			ndcPos.Z = clipPos.Z / clipPos.W;

			ClipMin.X = (std::min)(ClipMin.X, ndcPos.X);
			ClipMin.Y = (std::min)(ClipMin.Y, ndcPos.Y);
			ClipMin.Z = (std::min)(ClipMin.Z, ndcPos.Z);

			ClipMax.X = (std::max)(ClipMax.X, ndcPos.X);
			ClipMax.Y = (std::max)(ClipMax.Y, ndcPos.Y);
			ClipMax.Z = (std::max)(ClipMax.Z, ndcPos.Z);
		}
		
		BoundingVolumes[i] = { ClipMin, ClipMax };
	}

	// Update GPU buffer with new bounding volumes
	if (!BoundingVolumes.empty())
	{
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		HRESULT hr = InDeviceContext->Map(BoundingVolumeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		if (SUCCEEDED(hr))
		{
			memcpy(MappedResource.pData, BoundingVolumes.data(), BoundingVolumes.size() * sizeof(FBoundingVolume));
			InDeviceContext->Unmap(BoundingVolumeBuffer, 0);
		}
		else
		{
			// Handle error
		}
	}
}

void UOcclusionRenderer::DepthPrePass(ID3D11DeviceContext* InDeviceContext, UCamera* InCamera, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents)
{
	//// Save current device context state
	ID3D11RenderTargetView* pOldRTV = nullptr;
	ID3D11DepthStencilView* pOldDSV = nullptr;
	InDeviceContext->OMGetRenderTargets(1, &pOldRTV, &pOldDSV);

	ID3D11ShaderResourceView* pOldPSShaderResource = nullptr;
	InDeviceContext->PSGetShaderResources(0, 1, &pOldPSShaderResource);

	ID3D11PixelShader* pOldPS = nullptr;
	ID3D11ClassInstance* pOldPSInstances[256] = { 0 };
	UINT pOldPSNumInstances = 0;
	InDeviceContext->PSGetShader(&pOldPS, pOldPSInstances, &pOldPSNumInstances);

	ID3D11VertexShader* pOldVS = nullptr;
	ID3D11ClassInstance* pOldVSInstances[256] = { 0 };
	UINT pOldVSNumInstances = 0;
	InDeviceContext->VSGetShader(&pOldVS, pOldVSInstances, &pOldVSNumInstances);

	ID3D11InputLayout* pOldInputLayout = nullptr;
	InDeviceContext->IAGetInputLayout(&pOldInputLayout);

	D3D11_PRIMITIVE_TOPOLOGY OldPrimitiveTopology;
	InDeviceContext->IAGetPrimitiveTopology(&OldPrimitiveTopology);

	// Set render targets for depth pre-pass
	ID3D11RenderTargetView* NullRTV = nullptr;
	InDeviceContext->OMSetRenderTargets(0, &pOldRTV, DepthStencilView);

	// Clear the depth stencil view
	InDeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Set shaders and input layout for depth pre-pass
	InDeviceContext->VSSetShader(DepthVertexShader, nullptr, 0);
	InDeviceContext->PSSetShader(DepthPixelShader, nullptr, 0);
	InDeviceContext->IASetInputLayout(DepthInputLayout);

	// Loop through primitive components and draw them
	for (const auto& Primitive : InPrimitiveComponents)
	{
		if (!Primitive)
		{
			continue;
		}

		// Update constant buffer with WorldViewProj matrix
		FMatrix WorldMatrix = Primitive->GetWorldTransformMatrix();
		FViewProjConstants ViewProjConstants = InCamera->GetFViewProjConstants();
		FMatrix WorldViewProjMatrix = WorldMatrix * ViewProjConstants.View * ViewProjConstants.Projection;

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		HRESULT hResult = InDeviceContext->Map(DepthPassConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		if (SUCCEEDED(hResult))
		{
			memcpy(MappedResource.pData, &WorldViewProjMatrix, sizeof(FMatrix));
			InDeviceContext->Unmap(DepthPassConstantBuffer, 0);
		}
		else
		{
			// Handle error
		}

		InDeviceContext->VSSetConstantBuffers(0, 1, &DepthPassConstantBuffer);

		// Bind vertex and index buffers
		UINT stride = sizeof(FNormalVertex);
		UINT offset = 0;
		ID3D11Buffer* vertexBuffer = Primitive->GetVertexBuffer();
		ID3D11Buffer* indexBuffer = Primitive->GetIndexBuffer();

		InDeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		InDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Set primitive topology
		InDeviceContext->IASetPrimitiveTopology(Primitive->GetTopology());

		// Draw
		InDeviceContext->DrawIndexed(Primitive->GetNumIndices(), 0, 0);
	}

	// Restore previous device context state
	InDeviceContext->OMSetRenderTargets(1, &pOldRTV, pOldDSV);
	if (pOldRTV) pOldRTV->Release();
	if (pOldDSV) pOldDSV->Release();

	InDeviceContext->PSSetShaderResources(0, 1, &pOldPSShaderResource);
	if (pOldPSShaderResource) pOldPSShaderResource->Release();

	InDeviceContext->PSSetShader(pOldPS, pOldPSInstances, pOldPSNumInstances);
	if (pOldPS) pOldPS->Release();
	for (UINT i = 0; i < pOldPSNumInstances; ++i) if (pOldPSInstances[i]) pOldPSInstances[i]->Release();

	InDeviceContext->VSSetShader(pOldVS, pOldVSInstances, pOldVSNumInstances);
	if (pOldVS) pOldVS->Release();
	for (UINT i = 0; i < pOldVSNumInstances; ++i) if (pOldVSInstances[i]) pOldVSInstances[i]->Release();

	InDeviceContext->IASetInputLayout(pOldInputLayout);
	if (pOldInputLayout) pOldInputLayout->Release();

	InDeviceContext->IASetPrimitiveTopology(OldPrimitiveTopology);
}


void UOcclusionRenderer::GenerateHiZ(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	// Initial Copy (Mip 0): Copy depth buffer to HiZ Mip 0
	InDeviceContext->CSSetShader(HiZCopyDepthShader, nullptr, 0);
	InDeviceContext->CSSetShaderResources(0, 1, &DepthShaderResourceView);
	InDeviceContext->CSSetUnorderedAccessViews(0, 1, &HiZUnorderedAccessViews[0], nullptr);

	UINT Mip0Width = Width;
	UINT Mip0Height = Height;
	InDeviceContext->Dispatch((Mip0Width + 15) / 16, (Mip0Height + 15) / 16, 1);

	// Unbind resources
	ID3D11ShaderResourceView* NullSRV = nullptr;
	ID3D11UnorderedAccessView* NullUAV = nullptr;
	InDeviceContext->CSSetShaderResources(0, 1, &NullSRV);
	InDeviceContext->CSSetUnorderedAccessViews(0, 1, &NullUAV, nullptr);

	// Mipmap Generation Loop
	InDeviceContext->CSSetShader(HiZDownSampleShader, nullptr, 0);

	for (UINT Mip = 1; Mip < MipLevels; ++Mip)
	{
		UINT MipWidth = (std::max)(1u, Width >> Mip);
		UINT MipHeight = (std::max)(1u, Height >> Mip);

		// Update constant buffer
		FHiZDownsampleConstants constants;
		constants.TextureWidth = MipWidth;
		constants.TextureHeight = MipHeight;
		constants.MipLevel = Mip;
		constants.Padding = 0;

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		HRESULT hResult = InDeviceContext->Map(HiZDownsampleConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		if (SUCCEEDED(hResult))
		{
			memcpy(MappedResource.pData, &constants, sizeof(FHiZDownsampleConstants));
			InDeviceContext->Unmap(HiZDownsampleConstantBuffer, 0);
		}
		else
		{
			// Handle error
		}

		InDeviceContext->CSSetConstantBuffers(0, 1, &HiZDownsampleConstantBuffer);

		// Bind previous mip level as SRV, current mip level as UAV
		InDeviceContext->CSSetShaderResources(0, 1, &HiZShaderResourceViews[Mip - 1]);
		InDeviceContext->CSSetUnorderedAccessViews(0, 1, &HiZUnorderedAccessViews[Mip], nullptr);

		InDeviceContext->Dispatch((MipWidth + 15) / 16, (MipHeight + 15) / 16, 1);

		// Unbind resources
		InDeviceContext->CSSetShaderResources(0, 1, &NullSRV);
		InDeviceContext->CSSetUnorderedAccessViews(0, 1, &NullUAV, nullptr);
		ID3D11Buffer* NullCB = nullptr;
		InDeviceContext->CSSetConstantBuffers(0, 1, &NullCB);
	}

	// Final Cleanup
	InDeviceContext->CSSetShader(nullptr, nullptr, 0);
}


void UOcclusionRenderer::OcclusionTest(ID3D11Device* Device, ID3D11DeviceContext* InDeviceContext, UCamera* InCamera, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents, TArray<bool>& OutVisibilityResults)
{
	OutVisibilityResults.resize(InPrimitiveComponents.size());
	for (size_t i = 0; i < InPrimitiveComponents.size(); ++i)
	{
		OutVisibilityResults[i] = true;
	}

	if (InPrimitiveComponents.empty())
	{
		return;
	}

	BuildScreenSpaceBoundingVolumes(InDeviceContext, InCamera, InPrimitiveComponents);

	// Set occlusion shader
	InDeviceContext->CSSetShader(HiZOcclusionShader, nullptr, 0);

	// Update constant buffer
	FHiZOcclusionConstants constants;
	constants.NumBoundingVolumes = BoundingVolumes.size();
	constants.ScreenSize = FVector2((float)Width, (float)Height);
	constants.MipLevels = MipLevels;
	constants.Padding = FVector4::Zero();

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	HRESULT hr = InDeviceContext->Map(HiZOcclusionConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	if (SUCCEEDED(hr))
	{
		memcpy(MappedResource.pData, &constants, sizeof(FHiZOcclusionConstants));
		InDeviceContext->Unmap(HiZOcclusionConstantBuffer, 0);
	}
	else
	{
		// Handle error
		return;
	}

	InDeviceContext->CSSetConstantBuffers(0, 1, &HiZOcclusionConstantBuffer);

	// Bind resources
	InDeviceContext->CSSetShaderResources(0, 1, &BoundingVolumeShaderResourceView);
	InDeviceContext->CSSetShaderResources(1, 1, &HiZShaderResourceViews[MipLevels - 1]); // Smallest mip for occlusion test
	InDeviceContext->CSSetSamplers(0, 1, &HiZSamplerState);
	InDeviceContext->CSSetUnorderedAccessViews(0, 1, &VisibilityUnorderedAccessView, nullptr);

	// Dispatch compute shader
	UINT numGroups = (BoundingVolumes.size() + 63) / 64; // 64 threads per group
	InDeviceContext->Dispatch(numGroups, 1, 1);

	// Unbind resources
	ID3D11ShaderResourceView* NullSRV[2] = { nullptr, nullptr };
	ID3D11UnorderedAccessView* NullUAV = nullptr;
	ID3D11Buffer* NullCB = nullptr;
	ID3D11SamplerState* NullSampler = nullptr;
	InDeviceContext->CSSetShaderResources(0, 2, NullSRV);
	InDeviceContext->CSSetUnorderedAccessViews(0, 1, &NullUAV, nullptr);
	InDeviceContext->CSSetConstantBuffers(0, 1, &NullCB);
	InDeviceContext->CSSetSamplers(0, 1, &NullSampler);

	// Read back results
	InDeviceContext->CopyResource(VisibilityReadbackBuffer, VisibilityUAVBuffer);
	hr = InDeviceContext->Map(VisibilityReadbackBuffer, 0, D3D11_MAP_READ, 0, &MappedResource);
	if (SUCCEEDED(hr))
	{
		uint32* visibilityFlags = reinterpret_cast<uint32*>(MappedResource.pData);
		for (size_t i = 0; i < InPrimitiveComponents.size(); ++i)
		{
			if(i < BoundingVolumes.size())
				OutVisibilityResults[i] = (visibilityFlags[i] == 1);
		}
		InDeviceContext->Unmap(VisibilityReadbackBuffer, 0);
	}
	else
	{
		// Handle error
	}
}

void UOcclusionRenderer::CreateShader(ID3D11Device* InDevice)
{
	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	HRESULT hResult;

	// Compile HiZDownSampleCS.hlsl
	hResult = D3DCompileFromFile(
		L"Asset/Shader/HiZDownSampleCS.hlsl",
		nullptr,
		nullptr,
		"main",
		"cs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pShaderBlob,
		&pErrorBlob
	);

	if (FAILED(hResult))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}

	hResult = InDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, &HiZDownSampleShader);
	if (FAILED(hResult))
	{
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}
	if (pShaderBlob) pShaderBlob->Release();

	// Compile HiZOcclusionCS.hlsl
	pShaderBlob = nullptr;
	pErrorBlob = nullptr;
	hResult = D3DCompileFromFile(
		L"Asset/Shader/HiZOcclusionCS.hlsl",
		nullptr,
		nullptr,
		"main",
		"cs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pShaderBlob,
		&pErrorBlob
	);

	if (FAILED(hResult))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}

	hResult = InDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, &HiZOcclusionShader);
	if (FAILED(hResult))
	{
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}
	if (pShaderBlob) pShaderBlob->Release();

	// Compile HiZCopyDepthCS.hlsl
	pShaderBlob = nullptr;
	pErrorBlob = nullptr;
	hResult = D3DCompileFromFile(
		L"Asset/Shader/HiZCopyDepthCS.hlsl",
		nullptr,
		nullptr,
		"main",
		"cs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pShaderBlob,
		&pErrorBlob
	);

	if (FAILED(hResult))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}

	hResult = InDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, &HiZCopyDepthShader);
	if (FAILED(hResult))
	{
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}
	if (pShaderBlob) pShaderBlob->Release();

	// Compile DepthVS.hlsl
	pShaderBlob = nullptr;
	pErrorBlob = nullptr;
	hResult = D3DCompileFromFile(
		L"Asset/Shader/DepthVS.hlsl",
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pShaderBlob,
		&pErrorBlob
	);

	if (FAILED(hResult))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}

	hResult = InDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, &DepthVertexShader);
	if (FAILED(hResult))
	{
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}

	// Create input layout for DepthVS
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	hResult = InDevice->CreateInputLayout(layout, numElements, pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), &DepthInputLayout);
	if (FAILED(hResult))
	{
		if (pShaderBlob) pShaderBlob->Release();
		return;
	}
	if (pShaderBlob) pShaderBlob->Release();

	// Compile DepthPS.hlsl
	pShaderBlob = nullptr;
	pErrorBlob = nullptr;
	hResult = D3DCompileFromFile(
		L"Asset/Shader/DepthPS.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pShaderBlob,
		&pErrorBlob
	);

	if (FAILED(hResult))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		if (pShaderBlob) pShaderBlob->Release();
		// Log the error but don't return, allow subsequent resource creation to be attempted
		char buffer[256];
		sprintf_s(buffer, "Failed to compile DepthPS.hlsl. HRESULT: 0x%08X\n", hResult);
		OutputDebugStringA(buffer);
	}

	hResult = InDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, &DepthPixelShader);
	if (FAILED(hResult))
	{
		if (pShaderBlob) pShaderBlob->Release();
		// Log the error but don't return, allow subsequent resource creation to be attempted
		char buffer[256];
		sprintf_s(buffer, "Failed to create DepthPixelShader. HRESULT: 0x%08X\n", hResult);
		OutputDebugStringA(buffer);
	}
	if (pShaderBlob) pShaderBlob->Release();

	// Create DepthPassConstantBuffer
	D3D11_BUFFER_DESC CbDesc = {};
	CbDesc.ByteWidth = sizeof(FMatrix);
	CbDesc.Usage = D3D11_USAGE_DYNAMIC;
	CbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	CbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hResult = InDevice->CreateBuffer(&CbDesc, nullptr, &DepthPassConstantBuffer);
	if (FAILED(hResult))
	{
		OutputDebugStringA("Failed to create DepthPassConstantBuffer.\n");
		return;
	}

	// Create HiZSamplerState
	D3D11_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hResult = InDevice->CreateSamplerState(&SamplerDesc, &HiZSamplerState);
	if (FAILED(hResult))
	{
		OutputDebugStringA("Failed to create HiZSamplerState.\n");
		return;
	}

	// Create HiZOcclusionConstantBuffer
	D3D11_BUFFER_DESC HiZOcclusionCbDesc = {};
	HiZOcclusionCbDesc.ByteWidth = sizeof(FHiZOcclusionConstants);
	HiZOcclusionCbDesc.Usage = D3D11_USAGE_DYNAMIC;
	HiZOcclusionCbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	HiZOcclusionCbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hResult = InDevice->CreateBuffer(&HiZOcclusionCbDesc, nullptr, &HiZOcclusionConstantBuffer);
	if (FAILED(hResult))
	{
		return;
	}
}

void UOcclusionRenderer::CreateDepthResource(ID3D11Device* InDevice)
{
	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // to be viewable as depth and shader resource
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	HRESULT hResult = InDevice->CreateTexture2D(&TextureDesc, nullptr, &DepthTexture);
	if (FAILED(hResult))
	{
		char buffer[256];
		sprintf_s(buffer, "Failed to create DepthTexture. HRESULT: 0x%08X\n", hResult);
		OutputDebugStringA(buffer);
		return;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
	DepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	hResult = InDevice->CreateDepthStencilView(DepthTexture, &DepthStencilViewDesc, &DepthStencilView);
	if (FAILED(hResult))
	{
		char buffer[256];
		sprintf_s(buffer, "Failed to create DepthStencilView. HRESULT: 0x%08X\n", hResult);
		OutputDebugStringA(buffer);
		return;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
	ShaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	ShaderResourceViewDesc.Texture2D.MipLevels = 1;
	hResult = InDevice->CreateShaderResourceView(DepthTexture, &ShaderResourceViewDesc, &DepthShaderResourceView);
	if (FAILED(hResult))
	{
		char buffer[256];
		sprintf_s(buffer, "Failed to create DepthShaderResourceView. HRESULT: 0x%08X\n", hResult);
		OutputDebugStringA(buffer);
		return;
	}
}

void UOcclusionRenderer::CreateHiZResource(ID3D11Device* InDevice)
{
	MipLevels = static_cast<UINT>(floor(log2(max(Width, Height)))) + 1;

	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.MipLevels = MipLevels;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	HRESULT hResult = InDevice->CreateTexture2D(&TextureDesc, nullptr, &HiZTexture);
	if (FAILED(hResult))
	{
		return;
	}

	HiZShaderResourceViews.resize(MipLevels);
	HiZUnorderedAccessViews.resize(MipLevels);

	for (UINT i = 0; i < MipLevels; ++i)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
		ShaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
		ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		ShaderResourceViewDesc.Texture2D.MostDetailedMip = i;
		ShaderResourceViewDesc.Texture2D.MipLevels = 1;
		hResult = InDevice->CreateShaderResourceView(HiZTexture, &ShaderResourceViewDesc, &HiZShaderResourceViews[i]);
		if (FAILED(hResult))
		{
			return;
		}
		D3D11_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessViewDesc = {};
		UnorderedAccessViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
		UnorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		UnorderedAccessViewDesc.Texture2D.MipSlice = i;

		ID3D11UnorderedAccessView* UAV = nullptr;
		hResult = InDevice->CreateUnorderedAccessView(HiZTexture, &UnorderedAccessViewDesc, &HiZUnorderedAccessViews[i]);
		if (FAILED(hResult))
		{
			return;
		}
	}

	D3D11_BUFFER_DESC CbDesc = {};
	CbDesc.ByteWidth = sizeof(FHiZDownsampleConstants);
	CbDesc.Usage = D3D11_USAGE_DYNAMIC;
	CbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	CbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hResult = InDevice->CreateBuffer(&CbDesc, nullptr, &HiZDownsampleConstantBuffer);
	if (FAILED(hResult))
	{
		return;
	}
}


void UOcclusionRenderer::CreateBoundingVolumeResource(ID3D11Device* InDevice)
{
	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.ByteWidth = sizeof(FBoundingVolume) * 1024; // Assuming a max of 1024 bounding volumes for now
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	BufferDesc.StructureByteStride = sizeof(FBoundingVolume);

	HRESULT hResult = InDevice->CreateBuffer(&BufferDesc, nullptr, &BoundingVolumeBuffer);
	if (FAILED(hResult))
	{
		return;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
	ShaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	ShaderResourceViewDesc.BufferEx.FirstElement = 0;
	ShaderResourceViewDesc.BufferEx.NumElements = 1024;

	hResult = InDevice->CreateShaderResourceView(BoundingVolumeBuffer, &ShaderResourceViewDesc, &BoundingVolumeShaderResourceView);
	if (FAILED(hResult))
	{
		return;
	}
}

void UOcclusionRenderer::CreateVisibilityResource(ID3D11Device* InDevice)
{
	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.ByteWidth = sizeof(uint32) * 1024; // Assuming 1024 visibility flags for now
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	BufferDesc.StructureByteStride = sizeof(uint32);

	HRESULT hResult = InDevice->CreateBuffer(&BufferDesc, nullptr, &VisibilityUAVBuffer);
	if (FAILED(hResult))
	{
		return;
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessViewDesc = {};
	UnorderedAccessViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	UnorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	UnorderedAccessViewDesc.Buffer.FirstElement = 0;
	UnorderedAccessViewDesc.Buffer.NumElements = 1024;
	hResult = InDevice->CreateUnorderedAccessView(VisibilityUAVBuffer, &UnorderedAccessViewDesc, &VisibilityUnorderedAccessView);
	if (FAILED(hResult))
	{
		return;
	}

	D3D11_BUFFER_DESC ReadbackBufferDesc = {};
	ReadbackBufferDesc.ByteWidth = sizeof(uint32) * 1024;
	ReadbackBufferDesc.Usage = D3D11_USAGE_STAGING;
	ReadbackBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	ReadbackBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	ReadbackBufferDesc.StructureByteStride = sizeof(uint32);

	hResult = InDevice->CreateBuffer(&ReadbackBufferDesc, nullptr, &VisibilityReadbackBuffer);
	if (FAILED(hResult))
	{
		return;
	}
}

void UOcclusionRenderer::ReleaseShader()
{
	if (HiZDownSampleShader)
	{
		HiZDownSampleShader->Release();
		HiZDownSampleShader = nullptr;
	}
	if (HiZOcclusionShader)
	{
		HiZOcclusionShader->Release();
		HiZOcclusionShader = nullptr;
	}
	if (HiZCopyDepthShader)
	{
		HiZCopyDepthShader->Release();
		HiZCopyDepthShader = nullptr;
	}

	if (DepthVertexShader)
	{
		DepthVertexShader->Release();
		DepthVertexShader = nullptr;
	}
	if (DepthInputLayout)
	{
		DepthInputLayout->Release();
		DepthInputLayout = nullptr;
	}
	if (DepthPixelShader)
	{
		DepthPixelShader->Release();
		DepthPixelShader = nullptr;
	}
	if (DepthPassConstantBuffer)
	{
		DepthPassConstantBuffer->Release();
		DepthPassConstantBuffer = nullptr;
	}

	if (HiZSamplerState)
	{
		HiZSamplerState->Release();
		HiZSamplerState = nullptr;
	}
	if (HiZOcclusionConstantBuffer)
	{
		HiZOcclusionConstantBuffer->Release();
		HiZOcclusionConstantBuffer = nullptr;
	}
}

void UOcclusionRenderer::ReleaseDepthResource()
{
	if (DepthTexture)
	{
		DepthTexture->Release();
		DepthTexture = nullptr;
	}
	if (DepthStencilView)
	{
		DepthStencilView->Release();
		DepthStencilView = nullptr;
	}
	if (DepthShaderResourceView)
	{
		DepthShaderResourceView->Release();
		DepthShaderResourceView = nullptr;
	}
}

void UOcclusionRenderer::ReleaseHiZResource()
{
	if (HiZTexture)
	{
		HiZTexture->Release();
		HiZTexture = nullptr;
	}
	for (ID3D11ShaderResourceView* SRV : HiZShaderResourceViews)
	{
		SRV->Release();
	}
	for (ID3D11UnorderedAccessView* UAV : HiZUnorderedAccessViews)
	{
		UAV->Release();
	}
	HiZUnorderedAccessViews.clear();

	if (HiZDownsampleConstantBuffer)
	{
		HiZDownsampleConstantBuffer->Release();
		HiZDownsampleConstantBuffer = nullptr;
	}
}



void UOcclusionRenderer::ReleaseBoundingVolumeResource()
{
	if (BoundingVolumeBuffer)
	{
		BoundingVolumeBuffer->Release();	
		BoundingVolumeBuffer = nullptr;
	}
	if (BoundingVolumeShaderResourceView)
	{
		BoundingVolumeShaderResourceView->Release();
		BoundingVolumeShaderResourceView = nullptr;
	}
	if (HiZOcclusionConstantBuffer)
	{
		HiZOcclusionConstantBuffer->Release();
		HiZOcclusionConstantBuffer = nullptr;
	}
}

void UOcclusionRenderer::ReleaseVisibilityResource()
{
	if (VisibilityUAVBuffer)
	{
		VisibilityUAVBuffer->Release();
		VisibilityUAVBuffer = nullptr;
	}
	if (VisibilityUnorderedAccessView)
	{
		VisibilityUnorderedAccessView->Release();
		VisibilityUnorderedAccessView = nullptr;
	}
	if (VisibilityReadbackBuffer)
	{
		VisibilityReadbackBuffer->Release();
		VisibilityReadbackBuffer = nullptr;
	}
}
