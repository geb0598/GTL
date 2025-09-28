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
    uint Padding; // For 16-byte alignment
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

    // AABB가 클립 공간 밖에 있으면 무조건 보임 (오클루전 테스트 제외)
    if (minClip.x > 1.0f || maxClip.x < -1.0f || minClip.y > 1.0f || maxClip.y < -1.0f || minZ > 1.0f)
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

    // AABB의 중심점 UV 좌표 계산
    float2 centerClip = (minClip + maxClip) * 0.5f;
    float2 centerUV = ClipToTexCoord(centerClip);
    
    // Hi-Z 맵에서 가장 먼 깊이 값을 샘플링
    float occluderZ = HiZTexture.SampleLevel(Sampler_LinearClamp, centerUV, mip).r;

    // 객체의 가장 가까운 Z(minZ)가 가려지는 Z(occluderZ)보다 멀리 있으면 가려진 것
    if (minZ > occluderZ)
    {
        VisibilityBuffer[volumeIndex] = 0; // Occluded
    }
    else
    {
        VisibilityBuffer[volumeIndex] = 1; // Visible
    }
}
