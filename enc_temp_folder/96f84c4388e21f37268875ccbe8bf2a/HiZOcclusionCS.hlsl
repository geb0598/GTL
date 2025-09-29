struct BoundingVolume
{
    float4 Min; // (x, y, z, -) in clip space
    float4 Max; // (x, y, z, -) in clip space
};

StructuredBuffer<BoundingVolume> BoundingVolumes : register(t0);
Texture2D<float> HiZTexture : register(t1);
RWStructuredBuffer<uint> VisibilityBuffer : register(u0);

cbuffer CullingConstants : register(b0)
{
    uint NumBoundingVolumes;
    float2 ScreenSize;
    uint MipLevels;
};

SamplerState Sampler_LinearClamp : register(s0);

// Convert clip space coordinates (-1 to 1) to texture coordinates (0 to 1)
float2 ClipToTexCoord(float2 clipPos)
{
    return float2(clipPos.x * 0.5f + 0.5f, -clipPos.y * 0.5f + 0.5f);
}

[numthreads(64, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	uint volumeIndex = DispatchThreadID.x;
	if (volumeIndex >= NumBoundingVolumes)
	{
		return;
	}

	BoundingVolume bv = BoundingVolumes[volumeIndex];

	float2 minClip = bv.Min.xy;
	float2 maxClip = bv.Max.xy;
	float minZ = bv.Min.z;

    // Expand the screen-space bounding box slightly
	float2 expandAmount = 4.0f / ScreenSize; // 보수성을 약간 낮춰도 안정성이 확보된다면 성능에 유리
	minClip -= expandAmount;
	maxClip += expandAmount;

    // Clamp to NDC range [-1, 1]
	minClip = clamp(minClip, -1.0f, 1.0f);
	maxClip = clamp(maxClip, -1.0f, 1.0f);

    // AABB가 클립 공간 밖에 있으면 무조건 보임 (오클루전 테스트 제외)
	if (minClip.x >= 1.0f || maxClip.x <= -1.0f || minClip.y >= 1.0f || maxClip.y <= -1.0f || minZ >= 1.0f)
	{
		VisibilityBuffer[volumeIndex] = 1; // Visible
		return;
	}
    
    // 클립 공간 크기를 기반으로 적절한 Mip Level을 선택
	float2 clipSize = maxClip - minClip;
	float2 screenPixelSize = clipSize * 0.5f * ScreenSize;
	float maxDim = max(screenPixelSize.x, screenPixelSize.y);
	float mip = floor(log2(max(maxDim, 1.0f)));
	mip = clamp(mip, 0, MipLevels - 1);

	float dampening = 1.0 - smoothstep(0.975f, 0.999f, minZ);
	float depthFactor = minZ * 0.0045f * dampening;

	float epsilon = depthFactor;
    
	bool isVisible = false;

    // --- 제안하는 샘플링 패턴 (중앙 우선) ---
    // 1. 중앙 지점을 먼저 테스트
	float2 centerClip = (minClip + maxClip) * 0.5f;
	float2 centerUV = ClipToTexCoord(centerClip);
	float centerOccluderZ = HiZTexture.SampleLevel(Sampler_LinearClamp, centerUV, mip).r;

	if (minZ < centerOccluderZ + epsilon)
	{
		isVisible = true;
	}
	else
	{
        // 2. 중앙에서 가려졌다면, 그리드 전체를 순회하며 확인
		for (int y = 0; y < 5; ++y)
		{
			for (int x = 0; x < 5; ++x)
			{
				float2 sampleClip = lerp(minClip, maxClip, float2(x / 5.0f, y / 5.0f));
				float2 sampleUV = ClipToTexCoord(sampleClip);
				float occluderZ = HiZTexture.SampleLevel(Sampler_LinearClamp, sampleUV, mip).r;
                
				if (minZ < occluderZ + epsilon)
				{
					isVisible = true;
					break;
				}
			}
			if (isVisible)
				break;
		}
	}

	VisibilityBuffer[volumeIndex] = isVisible ? 1 : 0;
}
