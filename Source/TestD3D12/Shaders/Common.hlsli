#ifndef _COMMON_HLSLI_
#define _COMMON_HLSLI_

#include "ShaderInterop.h"

#if (__SHADER_TARGET_MAJOR > 6) || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 6)
    #define USE_DIRECT_DESCRIPTOR_HEAP
#endif

#ifdef USE_DIRECT_DESCRIPTOR_HEAP

#define ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED), "
#define ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES ""

Texture1D<float4> GetTexture1D(uint index) { return ResourceDescriptorHeap[index]; }
Texture1D<half4> GetTexture1DHalf4(uint index) { return ResourceDescriptorHeap[index]; }
Texture2D GetTexture2D(uint index) { return ResourceDescriptorHeap[index]; }
Texture2D<half4> GetTexture2DHalf4(uint index) { return ResourceDescriptorHeap[index]; }
Texture2D<float> GetTexture2DFloat(uint index) { return ResourceDescriptorHeap[index]; }
Texture2D<float2> GetTexture2DFloat2(uint index) { return ResourceDescriptorHeap[index]; }
Texture2D<uint> GetTexture2DUInt(uint index) { return ResourceDescriptorHeap[index]; }
Texture2D<uint4> GetTexture2DUInt4(uint index) { return ResourceDescriptorHeap[index]; }
Texture3D GetTexture3D(uint index) { return ResourceDescriptorHeap[index]; }
Texture3D<half4> GetTexture3DHalf4(uint index) { return ResourceDescriptorHeap[index]; }
TextureCube GetTextureCube(uint index) { return ResourceDescriptorHeap[index]; }
TextureCube<half4> GetTextureCubeHalf4(uint index) { return ResourceDescriptorHeap[index]; }
TextureCubeArray GetTextureCubeArray(uint index) { return ResourceDescriptorHeap[index]; }
ByteAddressBuffer GetBuffer(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<float> GetBufferFloat(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<float2> GetBufferFloat2(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<float3> GetBufferFloat3(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<float4> GetBufferFloat4(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<half> GetBufferFloat(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<half2> GetBufferFloat2(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<half3> GetBufferFloat3(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<half4> GetBufferFloat4(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<uint> GetBufferUInt(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<uint2> GetBufferUInt2(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<uint3> GetBufferUInt3(uint index) { return ResourceDescriptorHeap[index]; }
Buffer<uint4> GetBufferUInt4(uint index) { return ResourceDescriptorHeap[index]; }
StructuredBuffer<ShaderSurface> GetShaderSurfaceTable(uint index) { return ResourceDescriptorHeap[index]; }
StructuredBuffer<ShaderMaterial> GetShaderMaterialTable(uint index) { return ResourceDescriptorHeap[index]; }
RWTexture2D<float4> GetRWTexture2D(uint index) { return ResourceDescriptorHeap[index]; }
RWTexture2D<uint> GetRWTexture2DUInt(uint index) { return ResourceDescriptorHeap[index]; }
RWTexture2D<uint2> GetRWTexture2DUInt2(uint index) { return ResourceDescriptorHeap[index]; }
RWTexture2D<uint3> GetRWTexture2DUInt3(uint index) { return ResourceDescriptorHeap[index]; }
RWTexture2D<uint4> GetRWTexture2DUInt4(uint index) { return ResourceDescriptorHeap[index]; }
RWTexture3D<float4> GetRWTexture3D(uint index) { return ResourceDescriptorHeap[index]; }
RWTexture2DArray<float4> GetRWTexture2DArray(uint index) { return ResourceDescriptorHeap[index]; }
RWByteAddressBuffer GetRWBuffer(uint index) { return ResourceDescriptorHeap[index]; }
RWBuffer<float> GetRWBufferFloat(uint index) { return ResourceDescriptorHeap[index]; }
RWBuffer<float2> GetRWBufferFloat2(uint index) { return ResourceDescriptorHeap[index]; }
RWBuffer<float3> GetRWBufferFloat3(uint index) { return ResourceDescriptorHeap[index]; }
RWBuffer<float4> GetRWBufferFloat4(uint index) { return ResourceDescriptorHeap[index]; }
RWBuffer<uint> GetRWBufferUInt(uint index) { return ResourceDescriptorHeap[index]; }
RWBuffer<uint2> GetRWBufferUInt2(uint index) { return ResourceDescriptorHeap[index]; }
RWBuffer<uint3> GetRWBufferUInt3(uint index) { return ResourceDescriptorHeap[index]; }
RWBuffer<uint4> GetRWBufferUInt4(uint index) { return ResourceDescriptorHeap[index]; }
SamplerState GetSampler(uint index) { return SamplerDescriptorHeap[index]; }

#else

#define ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), "
#define ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 1, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 2, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 3, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 4, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 5, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 6, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 7, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 8, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 9, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 10, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 11, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 12, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 13, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 14, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 15, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 16, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 17, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 18, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 19, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 20, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 21, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 22, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 23, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 24, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 25, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 26, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 27, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(SRV(t0, numDescriptors = unbounded, space = 28, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 50, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 51, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 52, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 53, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 54, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 55, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 56, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 57, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 58, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 59, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 60, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 61, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 62, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 63, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 64, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(UAV(u0, numDescriptors = unbounded, space = 65, flags = DESCRIPTORS_VOLATILE)), " \
    "DescriptorTable(Sampler(s0, numDescriptors = unbounded, space = 66, flags = DESCRIPTORS_VOLATILE)), "

Texture1D<float4> bindlessTexture1DTable[] : register(t0, space1);
Texture1D<half4> bindlessTexture1DHalf4Table[] : register(t0, space2);
Texture2D bindlessTexture2DTable[] : register(t0, space3);
Texture2D<half4> bindlessTexture2DHalf4Table[] : register(t0, space4);
Texture2D<float> bindlessTexture2DFloatTable[] : register(t0, space5);
Texture2D<float2> bindlessTexture2DFloat2Table[] : register(t0, space6);
Texture2D<uint> bindlessTexture2DUIntTable[] : register(t0, space7);
Texture2D<uint4> bindlessTexture2DUInt4Table[] : register(t0, space8);
Texture3D bindlessTexture3DTable[] : register(t0, space9);
Texture3D<half4> bindlessTexture3DHalf4Table[] : register(t0, space10);
TextureCube bindlessTextureCubeTable[] : register(t0, space11);
TextureCube<half4> bindlessTextureCubeHalf4Table[] : register(t0, space12);
TextureCubeArray bindlessTextureCubeArrayTable[] : register(t0, space13);
ByteAddressBuffer bindlessBufferTable[] : register(t0, space14);
Buffer<float> bindlessBufferFloatTable[] : register(t0, space15);
Buffer<float2> bindlessBufferFloat2Table[] : register(t0, space16);
Buffer<float3> bindlessBufferFloat3Table[] : register(t0, space17);
Buffer<float4> bindlessBufferFloat4Table[] : register(t0, space18);
Buffer<half> bindlessBufferHalfTable[] : register(t0, space19);
Buffer<half2> bindlessBufferHalf2Table[] : register(t0, space20);
Buffer<half3> bindlessBufferHalf3Table[] : register(t0, space21);
Buffer<half4> bindlessBufferHalf4Table[] : register(t0, space22);
Buffer<uint> bindlessBufferUIntTable[] : register(t0, space23);
Buffer<uint2> bindlessBufferUInt2Table[] : register(t0, space24);
Buffer<uint3> bindlessBufferUInt3Table[] : register(t0, space25);
Buffer<uint4> bindlessBufferUInt4Table[] : register(t0, space26);
StructuredBuffer<ShaderSurface> bindlessShaderSurfaceTable[] : register(t0, space27);
StructuredBuffer<ShaderMaterial> bindlessShaderMaterialTable[] : register(t0, space28);
RWTexture2D<float4> bindlessRWTexture2DTable[] : register(u0, space50);
RWTexture2D<uint> bindlessRWTexture2DUIntTable[] : register(u0, space51);
RWTexture2D<uint2> bindlessRWTexture2DUInt2Table[] : register(u0, space52);
RWTexture2D<uint3> bindlessRWTexture2DUInt3Table[] : register(u0, space53);
RWTexture2D<uint4> bindlessRWTexture2DUInt4Table[] : register(u0, space54);
RWTexture3D<float4> bindlessRWTexture3DTable[] : register(u0, space55);
RWTexture2DArray<float4> bindlessRWTexture2DArrayTable[] : register(u0, space56);
RWByteAddressBuffer bindlessRWBufferTable[] : register(u0, space57);
RWBuffer<float> bindlessRWBufferFloatTable[] : register(u0, space58);
RWBuffer<float2> bindlessRWBufferFloat2Table[] : register(u0, space59);
RWBuffer<float3> bindlessRWBufferFloat3Table[] : register(u0, space60);
RWBuffer<float4> bindlessRWBufferFloat4Table[] : register(u0, space61);
RWBuffer<uint> bindlessRWBufferUIntTable[] : register(u0, space62);
RWBuffer<uint2> bindlessRWBufferUInt2Table[] : register(u0, space63);
RWBuffer<uint3> bindlessRWBufferUInt3Table[] : register(u0, space64);
RWBuffer<uint4> bindlessRWBufferUInt4Table[] : register(u0, space65);
SamplerState bindlessSamplerTable[] : register(s0, space66);

Texture1D<float4> GetTexture1D(uint index) { return bindlessTexture1DTable[index]; }
Texture1D<half4> GetTexture1DHalf4(uint index) { return bindlessTexture1DHalf4Table[index]; }
Texture2D GetTexture2D(uint index) { return bindlessTexture2DTable[index]; }
Texture2D<half4> GetTexture2DHalf4(uint index) { return bindlessTexture2DHalf4Table[index]; }
Texture2D<float> GetTexture2DFloat(uint index) { return bindlessTexture2DFloatTable[index]; }
Texture2D<float2> GetTexture2DFloat2(uint index) { return bindlessTexture2DFloat2Table[index]; }
Texture2D<uint> GetTexture2DUInt(uint index) { return bindlessTexture2DUIntTable[index]; }
Texture2D<uint4> GetTexture2DUInt4(uint index) { return bindlessTexture2DUInt4Table[index]; }
Texture3D GetTexture3D(uint index) { return bindlessTexture3DTable[index]; }
Texture3D<half4> GetTexture3DHalf4(uint index) { return bindlessTexture3DHalf4Table[index]; }
TextureCube GetTextureCube(uint index) { return bindlessTextureCubeTable[index]; }
TextureCube<half4> GetTextureCubeHalf4(uint index) { return bindlessTextureCubeHalf4Table[index]; }
TextureCubeArray GetTextureCubeArray(uint index) { return bindlessTextureCubeArrayTable[index]; }
ByteAddressBuffer GetBuffer(uint index) { return bindlessBufferTable[index]; }
Buffer<float> GetBufferFloat(uint index) { return bindlessBufferFloatTable[index]; }
Buffer<float2> GetBufferFloat2(uint index) { return bindlessBufferFloat2Table[index]; }
Buffer<float3> GetBufferFloat3(uint index) { return bindlessBufferFloat3Table[index]; }
Buffer<float4> GetBufferFloat4(uint index) { return bindlessBufferFloat4Table[index]; }
Buffer<half> GetBufferHalf(uint index) { return bindlessBufferHalfTable[index]; }
Buffer<half2> GetBufferHalf2(uint index) { return bindlessBufferHalf2Table[index]; }
Buffer<half3> GetBufferHalf3(uint index) { return bindlessBufferHalf3Table[index]; }
Buffer<half4> GetBufferHalf4(uint index) { return bindlessBufferHalf4Table[index]; }
Buffer<uint> GetBufferUInt(uint index) { return bindlessBufferUIntTable[index]; }
Buffer<uint2> GetBufferUInt2(uint index) { return bindlessBufferUInt2Table[index]; }
Buffer<uint3> GetBufferUInt3(uint index) { return bindlessBufferUInt3Table[index]; }
Buffer<uint4> GetBufferUInt4(uint index) { return bindlessBufferUInt4Table[index]; }
StructuredBuffer<ShaderSurface> GetShaderSurfaceTable(uint index) { return bindlessShaderSurfaceTable[index]; }
StructuredBuffer<ShaderMaterial> GetShaderMaterialTable(uint index) { return bindlessShaderMaterialTable[index]; }
RWTexture2D<float4> GetRWTexture2D(uint index) { return bindlessRWTexture2DTable[index]; }
RWTexture2D<uint> GetRWTexture2DUInt(uint index) { return bindlessRWTexture2DUIntTable[index]; }
RWTexture2D<uint2> GetRWTexture2DUInt2(uint index) { return bindlessRWTexture2DUInt2Table[index]; }
RWTexture2D<uint3> GetRWTexture2DUInt3(uint index) { return bindlessRWTexture2DUInt3Table[index]; }
RWTexture2D<uint4> GetRWTexture2DUInt4(uint index) { return bindlessRWTexture2DUInt4Table[index]; }
RWTexture3D<float4> GetRWTexture3D(uint index) { return bindlessRWTexture3DTable[index]; }
RWTexture2DArray<float4> GetRWTexture2DArray(uint index) { return bindlessRWTexture2DArrayTable[index]; }
RWByteAddressBuffer GetRWBuffer(uint index) { return bindlessRWBufferTable[index]; }
RWBuffer<float> GetRWBufferFloat(uint index) { return bindlessRWBufferFloatTable[index]; }
RWBuffer<float2> GetRWBufferFloat2(uint index) { return bindlessRWBufferFloat2Table[index]; }
RWBuffer<float3> GetRWBufferFloat3(uint index) { return bindlessRWBufferFloat3Table[index]; }
RWBuffer<float4> GetRWBufferFloat4(uint index) { return bindlessRWBufferFloat4Table[index]; }
RWBuffer<uint> GetRWBufferUInt(uint index) { return bindlessRWBufferUIntTable[index]; }
RWBuffer<uint2> GetRWBufferUInt2(uint index) { return bindlessRWBufferUInt2Table[index]; }
RWBuffer<uint3> GetRWBufferUInt3(uint index) { return bindlessRWBufferUInt3Table[index]; }
RWBuffer<uint4> GetRWBufferUInt4(uint index) { return bindlessRWBufferUInt4Table[index]; }
SamplerState GetSampler(uint index) { return bindlessSamplerTable[index]; }

#endif

#define ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s101, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s102, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s103, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT)," \
    "StaticSampler(s104, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_POINT)," \
    "StaticSampler(s105, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_POINT)," \
    "StaticSampler(s106, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
    "StaticSampler(s107, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
    "StaticSampler(s108, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)"

#define DECLARE_COMMON_STATIC_SAMPLERS \
    SamplerState linearClampSampler : register(s100); \
    SamplerState linearWrapSampler : register(s101); \
    SamplerState linearMirrorSampler : register(s102); \
    SamplerState pointClampSampler : register(s103); \
    SamplerState pointWrapSampler : register(s104); \
    SamplerState pointMirrorSampler : register(s105); \
    SamplerState anisoClampSampler : register(s106); \
    SamplerState anisoWrapSampler : register(s107); \
    SamplerState anisoMirrorSampler : register(s108);

inline float ComputeTextureLod(in uint2 textureSize, in float2 dFdx_uv, in float2 dFdy_uv, uint anisotropy = 0) {
    // Isotropic:
    if (anisotropy == 0) {
        return log2(max(length(dFdx_uv * textureSize), length(dFdy_uv * textureSize)));
    }

    // Anisotropic:
    dFdx_uv *= textureSize;
    dFdy_uv *= textureSize;
    float squaredLengthX = dot(dFdx_uv, dFdx_uv);
    float squaredLengthY = dot(dFdy_uv, dFdy_uv);
    float determinant = abs(dFdx_uv.x * dFdy_uv.y - dFdx_uv.y * dFdy_uv.x);
    float squaredLengthMajor = squaredLengthX > squaredLengthY ? squaredLengthX : squaredLengthY;
    float lengthMajor = sqrt(squaredLengthMajor);
    float ratioOfAnisotropy = squaredLengthMajor / determinant;
    float lengthMinor = 0;
    if (ratioOfAnisotropy > anisotropy) {
        // ratio is clamped - LOD is based on ratio (preserves area)
        lengthMinor = lengthMajor / anisotropy;
    } else {
        // ratio not clamped - LOD is based on area
        lengthMinor = determinant / lengthMajor;
    }
    return log2(lengthMinor);
}

inline void WriteMipmapFeedback(RWBuffer<uint> feedbackBuffer, in uint materialIndex, in float2 dFdx_uv, in float2 dFdy_uv) {
    const float lod = ComputeTextureLod(TextureStreamingFeedbackResolution, dFdx_uv, dFdy_uv);
    const uint clampedLod = (uint)clamp(lod, 0.0f, (float)TextureStreamingFeedbackMaxLOD);
    const uint resolution = TextureStreamingFeedbackResolution >> clampedLod;
    const uint resolutionMask = WaveActiveBitOr(resolution);
    
    if (WaveIsFirstLane()) {
        InterlockedOr(feedbackBuffer[materialIndex], resolutionMask);
    }
}

#endif
