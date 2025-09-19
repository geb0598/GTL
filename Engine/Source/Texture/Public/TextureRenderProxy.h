#pragma once
#include "pch.h"

struct FTextureRenderProxy
{
public:
	ID3D11ShaderResourceView* GetSRV() const { return SRV; }
	ID3D11SamplerState* GetSampler() const { return Sampler; }

private:
	ID3D11ShaderResourceView* SRV = nullptr;
	ID3D11SamplerState* Sampler = nullptr;
	uint32 Width = 0;
	uint32 Height = 0;
};
