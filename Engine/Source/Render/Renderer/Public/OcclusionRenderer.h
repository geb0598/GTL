#pragma once

#include <d3d11.h>

#include "Global/Types.h"
#include "Global/Vector.h"
#include "Core/Public/Object.h"
#include "Core/Public/ObjectPtr.h"

class UPrimitiveComponent;

struct FBoundingVolume
{
	FVector Min;
	FVector Max;
};

struct FHiZDownsampleConstants
{
	UINT TextureWidth;
	UINT TextureHeight;
	UINT MipLevel;
	UINT Padding;
};

UCLASS()
class UOcclusionRenderer :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UOcclusionRenderer, UObject)
public:
	void Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, uint32 InWidth, uint32 InHeight);

	void Release();

	void Resize(uint32 InWidth, uint32 InHeight)
	{
		Width = InWidth;
		Height = InHeight;
	}

	void BuildScreenSpaceBoundingVolumes(ID3D11DeviceContext* InDeviceContext, UCamera* InCamera, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents);

	void DepthPrePass(ID3D11DeviceContext* InDeviceContext, UCamera* InCamera, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents);

	void GenerateHiZ(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);

	void OcclusionTest(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, UCamera* InCamera, const TArray<TObjectPtr<UPrimitiveComponent>>& InPrimitiveComponents, TArray<bool>& OutVisibilityResults);

private:
	static constexpr size_t NUM_WORKER_THREADS = 4;

	void CreateShader(ID3D11Device* InDevice);
	void CreateDepthResource(ID3D11Device* InDevice);
	void CreateHiZResource(ID3D11Device* InDevice);
	void CreateVisibilityResource(ID3D11Device* InDevice);

	void ReleaseShader();
	void ReleaseDepthResource();
	void ReleaseHiZResource();
	void ReleaseVisibilityResource();

	/** @note: UOcclusionRenderer에서는 Device와 DeviceContext의 수명을 관리하지 않음*/
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;

	/** @brief: Depth resources */
	ID3D11Texture2D* DepthTexture = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	ID3D11ShaderResourceView* DepthShaderResourceView = nullptr;

	/** @brief: HiZ resources */
	ID3D11Texture2D* HiZTexture = nullptr;
	TArray<ID3D11ShaderResourceView*> HiZShaderResourceViews;
	TArray<ID3D11UnorderedAccessView*> HiZUnorderedAccessViews;

	ID3D11Buffer* HiZDownsampleConstantBuffer = nullptr;

	ID3D11ComputeShader* HiZDownSampleShader = nullptr;
	ID3D11ComputeShader* HiZOcclusionShader = nullptr;
	ID3D11ComputeShader* HiZCopyDepthShader = nullptr;

	ID3D11VertexShader* DepthVertexShader = nullptr;
	ID3D11InputLayout* DepthInputLayout = nullptr;
	ID3D11PixelShader* DepthPixelShader = nullptr;

	ID3D11Buffer* DepthPassConstantBuffer = nullptr;

	UINT MipLevels;

	struct FHiZOcclusionConstants
	{
		UINT NumBoundingVolumes;
		FVector2 ScreenSize;
		UINT MipLevels;
		FVector4 Padding; // For 16-byte alignment
	};

	/** @brief: Boudning volume resources */
	[[deprecated]] ID3D11Buffer* BoundingVolumeBuffer = nullptr;
	[[deprecated]] ID3D11ShaderResourceView* BoundingVolumeShaderResourceView = nullptr;
	TArray<FBoundingVolume> BoundingVolumes;

	ID3D11Buffer* HiZOcclusionConstantBuffer = nullptr;
	ID3D11SamplerState* HiZSamplerState = nullptr;

	/** @brief: Visibility resources */
	ID3D11Buffer* VisibilityUAVBuffer = nullptr;
	ID3D11UnorderedAccessView* VisibilityUnorderedAccessView = nullptr;
	ID3D11Buffer* VisibilityReadbackBuffer = nullptr;

	uint32 Width;
	uint32 Height;

	//ThreadPool Pool;
};
