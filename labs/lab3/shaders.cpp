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
    float4 Pos : POSITION;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.Pos = mul(input.Pos, mWorld);
    output.Pos = mul(output.Pos, mView);
    output.Pos = mul(output.Pos, mProjection);
    output.Color = input.Color;
    return output;
}
)";

const char* pixelShaderCode = R"(
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

float4 main(PS_INPUT input) : SV_Target
{
    return input.Color;
}
)";