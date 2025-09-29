cbuffer cb0 : register(b0)
{
    uint2 g_TextureSize; // Size of the current mip level (output)
    uint g_MipLevel;    // Current mip level being generated
    uint g_Padding;
};

Texture2D<float> g_InputTexture : register(t0); // Previous mip level
RWTexture2D<float> g_OutputTexture : register(u0); // Current mip level

SamplerState Sampler_LinearClamp : register(s0); // Declare a sampler state

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Calculate the UV for the current pixel in the output mip level
    // Each pixel in the current mip level corresponds to a 2x2 block in the previous mip level.
    // We need to sample the four corners of this 2x2 block.

    // Coordinates in the previous mip level
    float2 prevMipCoord = (DTid.xy * 2.0f); // Top-left corner of the 2x2 block in previous mip

    // Sample 4 texels from the previous mip level
    float depth0 = g_InputTexture.SampleLevel(Sampler_LinearClamp, (prevMipCoord + float2(0.5f, 0.5f)) / (float2)(g_TextureSize * 2), g_MipLevel - 1).r;
    float depth1 = g_InputTexture.SampleLevel(Sampler_LinearClamp, (prevMipCoord + float2(1.5f, 0.5f)) / (float2)(g_TextureSize * 2), g_MipLevel - 1).r;
    float depth2 = g_InputTexture.SampleLevel(Sampler_LinearClamp, (prevMipCoord + float2(0.5f, 1.5f)) / (float2)(g_TextureSize * 2), g_MipLevel - 1).r;
    float depth3 = g_InputTexture.SampleLevel(Sampler_LinearClamp, (prevMipCoord + float2(1.5f, 1.5f)) / (float2)(g_TextureSize * 2), g_MipLevel - 1).r;

    float maxDepth = max(max(depth0, depth1), max(depth2, depth3));

    g_OutputTexture[DTid.xy] = maxDepth;
}
