#ifndef SHADERS_H
#define SHADERS_H

#include "main.h"

extern const char* vertexShaderCode;
extern const char* pixelShaderCode;

struct SimpleVertex {
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

struct ConstantBufferWorld {
    DirectX::XMMATRIX mWorld;
};

struct ConstantBufferViewProjection {
    DirectX::XMMATRIX mView;
    DirectX::XMMATRIX mProjection;
};

#endif