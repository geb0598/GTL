cbuffer constants : register(b0)
{
	row_major float4x4 world;
}

cbuffer PerFrame : register(b1)
{
	row_major float4x4 View;		// View Matrix Calculation of MVP Matrix
	row_major float4x4 Projection;	// Projection Matrix Calculation of MVP Matrix
};

cbuffer PerFrame : register(b2)
{
	float4 totalColor;
};

Texture2D DiffuseTexture : register(t0);
SamplerState SamplerWrap : register(s0);

struct VS_INPUT
{
	float4 position : POSITION;		// Input position from vertex buffer
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;	// Transformed position to pass to the pixel shader
	float2 tex : TEXCOORD0;
};

PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output;
	float4 tmp = input.position;
	tmp = mul(tmp, world);
	tmp = mul(tmp, View);
	tmp = mul(tmp, Projection);

	output.position = tmp;
	output.tex = input.tex;

	return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
	float4 texColor = DiffuseTexture.Sample(SamplerWrap, input.tex);
	float4 finalColor = lerp(texColor, totalColor, totalColor.a);

	return finalColor;
}
