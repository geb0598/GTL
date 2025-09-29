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

float2 MyLerp(float2 a, float2 b, float2 t)
{
	return a + t * (b - a);
}


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

    // AABB가 클립 공간 밖에 있으면 무조건 보임 (오클루전 테스트 제외)
	if (minClip.x >= 1.0f || maxClip.x <= -1.0f || minClip.y >= 1.0f || maxClip.y <= -1.0f || minZ >= 1.0f)
	{
		VisibilityBuffer[volumeIndex] = 1; // Visible
		return;
	}
    
    // 클립 공간 크기를 기반으로 적절한 Mip Level을 선택
	float2 clipSize = maxClip - minClip;
	float2 screenPixelSize = clipSize * ScreenSize;
	float maxDim = max(screenPixelSize.x, screenPixelSize.y);
	float mip = log2(max(maxDim, 1.0f)) + 2.5f;
	mip = clamp(mip, 0, MipLevels - 1);

    // AABB의 클립 공간을 3x3 그리드로 나누어 UV 좌표 계산
	bool isVisible = false;
	for (int y = 0; y <= 5; ++y)
	{
		for (int x = 0; x <= 5; ++x)
		{
			float2 t = float2(x / 5.0f, y / 5.0f);
			float2 sampleClip = MyLerp(minClip, maxClip, t);
			float2 sampleUV = ClipToTexCoord(sampleClip);
			float occluderZ = HiZTexture.SampleLevel(Sampler_LinearClamp, sampleUV, mip);
			if (minZ <= occluderZ) // 객체의 가장 가까운 Z가 Hi-Z 맵의 깊이보다 가까우면 가시적
			{
				isVisible = true;
				break;
			}
		}
		if (isVisible)
			break;
	}

	VisibilityBuffer[volumeIndex] = isVisible ? 1 : 0;
}
