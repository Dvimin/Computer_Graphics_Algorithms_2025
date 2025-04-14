#ifndef SHADERS_H
#define SHADERS_H

#include "main.h"

extern const char* vertexShaderCode;
extern const char* pixelShaderCode;
extern const char* skyboxVertexShaderCode;
extern const char* skyboxPixelShaderCode;

struct SimpleVertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 tex;
};

struct ConstantBufferWorld {
    DirectX::XMMATRIX mWorld;
};

struct ConstantBufferViewProjection {
    DirectX::XMMATRIX mView;
    DirectX::XMMATRIX mProjection;
};

#endif

