#include "shaders.h"

const char* vertexShaderCode = R"(
cbuffer ConstantBufferWorld : register(b0)
{
    matrix mWorld;
};

cbuffer ConstantBufferViewProjection : register(b1)
{
    matrix mView;
    matrix mProjection;
};

struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 UV  : TEXCOORD;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 UV  : TEXCOORD;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    float4 pos = float4(input.Pos, 1.0f);
    pos = mul(pos, mWorld);
    pos = mul(pos, mView);
    pos = mul(pos, mProjection);
    output.Pos = pos;
    output.UV = input.UV;
    return output;
}
)";


const char* pixelShaderCode =
"Texture2D tex : register(t0);"
"SamplerState sam : register(s0);"
"float4 main(float4 pos : SV_POSITION, float2 texCoord : TEXCOORD) : SV_TARGET {"
"    return tex.Sample(sam, texCoord);" 
"}";

const char* skyboxVertexShaderCode = R"(
cbuffer ViewProjection : register(b0)
{
    matrix view;
    matrix projection;
};

struct VS_INPUT
{
    float3 Pos : POSITION;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Pos = mul(float4(input.Pos, 1.0f), view);
    output.Pos = mul(output.Pos, projection);
    output.TexCoord = input.Pos;
    return output;
}
)";

const char* skyboxPixelShaderCode = R"(
TextureCube skyboxTexture : register(t0);
SamplerState skyboxSampler : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_Target
{
    return skyboxTexture.Sample(skyboxSampler, input.TexCoord);
}
)";