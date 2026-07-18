#include "ShaderInterop.h"

#define DEBUG_DRAW_ROOT_SIGNATURE \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
    "CBV(b1)"

struct VSInput {
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

[RootSignature(DEBUG_DRAW_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;
    result.position = mul(cameraConstants.viewProjMatrix, float4(input.position, 1.0));
    result.color = input.color;
    return result;
}

[RootSignature(DEBUG_DRAW_ROOT_SIGNATURE)]
float4 PSMain(PSInput input) : SV_Target0 {
    return input.color;
}
