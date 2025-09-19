cbuffer constants : register(b0)
{
	row_major float4x4 WorldMatrix;
}

cbuffer ViewProjectionBuffer : register(b1)
{
	row_major float4x4 View; // View Matrix Calculation of MVP Matrix
	row_major float4x4 Projection; // Projection Matrix Calculation of MVP Matrix
};

// StaticMeshShader
struct VS_INPUT
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float2 TexCoord : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float4 Color : COLOR;
};

// 전역 텍스처 및 샘플러
Texture2D BaseColorTexture : register(t0);
SamplerState BaseColorSampler : register(s0);

PS_INPUT mainVS(VS_INPUT Input)
{
	PS_INPUT Output;
	float4 Tmp = Input.Position;
	Tmp = mul(Tmp, WorldMatrix);
	Tmp = mul(Tmp, View);
	Tmp = mul(Tmp, Projection);

	Output.Position = Tmp;
	Output.TexCoord = Input.TexCoord;
	Output.Color = Input.Color;

	return Output;
}

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
	float4 FinalColor = BaseColorTexture.Sample(BaseColorSampler, Input.TexCoord);
    
	return FinalColor * Input.Color;
}
