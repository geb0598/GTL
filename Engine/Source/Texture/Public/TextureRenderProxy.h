#pragma once
#include "pch.h"

struct FTextureRenderProxy
{
public:
	FTextureRenderProxy(ID3D11ShaderResourceView* InSRV, ID3D11SamplerState* InSampler, uint32 InWidth = 0, uint32 InHeight = 0)
		: SRV(InSRV), Sampler(InSampler), Width(InWidth), Height(InHeight) {}

	ID3D11ShaderResourceView* GetSRV() const { return SRV; }
	ID3D11SamplerState* GetSampler() const { return Sampler; }

private:
	ID3D11ShaderResourceView* SRV = nullptr;
	ID3D11SamplerState* Sampler = nullptr;
	uint32 Width = 0;
	uint32 Height = 0;
};
