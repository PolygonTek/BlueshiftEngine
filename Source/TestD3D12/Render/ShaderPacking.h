// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cmath>
#include <cstdint>
#include <DirectXPackedVector.h>

#include "Shaders/ShaderInterop.h"

// Common.hlsli 와 같은 함수명과 bit layout 을 사용해 CPU 에서 shader parameter 를 패킹한다.
class ShaderPacking {
public:
                        /// Unit vector3 <-> RGB8 unorm
    static uint         PackUnitVector3ToRGB8UNorm(const float3 &value);
    static float3       UnpackRGB8UNormToUnitVector3(uint value);

                        /// Unit vector4 <-> RGBA8 unorm
    static uint         PackUnitVector4ToRGBA8UNorm(const float4 &value);
    static float4       UnpackRGBA8UNormToUnitVector4(uint value);

                        /// Float4 <-> RGBA8 unorm
    static uint         PackFloat4ToRGBA8UNorm(const float4 &value);
    static float4       UnpackRGBA8UNormToFloat4(uint value);

                        /// Float4 <-> RGB10A2 unorm
    static uint         PackFloat4ToRGB10A2UNorm(const float4 &value);
    static float4       UnpackRGB10A2UNormToFloat4(uint value);

                        /// Float2 <-> RG16F
    static uint         PackFloat2ToRG16F(const float2 &value) { return FloatToHalfBits(value.x) | (FloatToHalfBits(value.y) << 16u); }
    static float2       UnpackRG16FToFloat2(uint value) { return float2(HalfBitsToFloat(value), HalfBitsToFloat(value >> 16u)); }

                        /// Float3 <-> RGB16F
    static uint2        PackFloat3ToRGB16F(const float3 &value) { return uint2(PackFloat2ToRG16F(float2(value.x, value.y)), FloatToHalfBits(value.z)); }
    static float3       UnpackRGB16FToFloat3(const uint2 &value);

                        /// Float4 <-> RGBA16F
    static uint2        PackFloat4ToRGBA16F(const float4 &value) { return uint2(PackFloat2ToRG16F(float2(value.x, value.y)), PackFloat2ToRG16F(float2(value.z, value.w))); }
    static float4       UnpackRGBA16FToFloat4(const uint2 &value);

                        /// Uint2 <-> RG16 uint
    static uint         PackUint2ToRG16Uint(const uint2 &value) { return (value.x & 0xFFFFu) | ((value.y & 0xFFFFu) << 16u); }
    static uint2        UnpackRG16UintToUint2(uint value) { return uint2(value & 0xFFFFu, value >> 16u); }

                        /// Float2 <-> RG16 unorm
    static uint         PackFloat2ToRG16UNorm(const float2 &value) { return PackUint2ToRG16Uint(uint2(QuantizeUNorm(value.x, 65535u), QuantizeUNorm(value.y, 65535u))); }
    static float2       UnpackRG16UNormToFloat2(uint value) { return float2(DequantizeUNorm(value & 0xFFFFu, 65535u), DequantizeUNorm(value >> 16u, 65535u)); }

                        /// Float4 <-> RGBA16 unorm
    static uint2        PackFloat4ToRGBA16UNorm(const float4 &value) { return uint2(PackFloat2ToRG16UNorm(float2(value.x, value.y)), PackFloat2ToRG16UNorm(float2(value.z, value.w))); }
    static float4       UnpackRGBA16UNormToFloat4(const uint2 &value);

private:
    static float        Saturate(float value) { return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value); }
    static uint         QuantizeUNorm(float value, uint maxValue) { return static_cast<uint>(std::floor(Saturate(value) * static_cast<float>(maxValue) + 0.5f)); }
    static float        DequantizeUNorm(uint value, uint maxValue) { return static_cast<float>(value) / static_cast<float>(maxValue); }
    static uint         FloatToHalfBits(float value) { return static_cast<uint>(DirectX::PackedVector::XMConvertFloatToHalf(value)); }
    static float        HalfBitsToFloat(uint value) { return DirectX::PackedVector::XMConvertHalfToFloat(static_cast<DirectX::PackedVector::HALF>(value & 0xFFFFu)); }
};

BE_INLINE uint ShaderPacking::PackUnitVector3ToRGB8UNorm(const float3 &value) {
    return QuantizeUNorm(value.x * 0.5f + 0.5f, 255u) |
          (QuantizeUNorm(value.y * 0.5f + 0.5f, 255u) << 8u) |
          (QuantizeUNorm(value.z * 0.5f + 0.5f, 255u) << 16u);
}

BE_INLINE float3 ShaderPacking::UnpackRGB8UNormToUnitVector3(uint value) {
    return float3(DequantizeUNorm(value & 0xFFu, 255u) * 2.0f - 1.0f,
                  DequantizeUNorm((value >> 8u) & 0xFFu, 255u) * 2.0f - 1.0f,
                  DequantizeUNorm((value >> 16u) & 0xFFu, 255u) * 2.0f - 1.0f);
}

BE_INLINE uint ShaderPacking::PackUnitVector4ToRGBA8UNorm(const float4 &value) {
    return QuantizeUNorm(value.x * 0.5f + 0.5f, 255u) |
          (QuantizeUNorm(value.y * 0.5f + 0.5f, 255u) << 8u) |
          (QuantizeUNorm(value.z * 0.5f + 0.5f, 255u) << 16u) |
          (QuantizeUNorm(value.w * 0.5f + 0.5f, 255u) << 24u);
}

BE_INLINE float4 ShaderPacking::UnpackRGBA8UNormToUnitVector4(uint value) {
    return float4(DequantizeUNorm(value & 0xFFu, 255u) * 2.0f - 1.0f,
                  DequantizeUNorm((value >> 8u) & 0xFFu, 255u) * 2.0f - 1.0f,
                  DequantizeUNorm((value >> 16u) & 0xFFu, 255u) * 2.0f - 1.0f,
                  DequantizeUNorm(value >> 24u, 255u) * 2.0f - 1.0f);
}

BE_INLINE uint ShaderPacking::PackFloat4ToRGBA8UNorm(const float4 &value) {
    return QuantizeUNorm(value.x, 255u) |
          (QuantizeUNorm(value.y, 255u) << 8u) |
          (QuantizeUNorm(value.z, 255u) << 16u) |
          (QuantizeUNorm(value.w, 255u) << 24u);
}

BE_INLINE float4 ShaderPacking::UnpackRGBA8UNormToFloat4(uint value) {
    return float4(DequantizeUNorm(value & 0xFFu, 255u),
                  DequantizeUNorm((value >> 8u) & 0xFFu, 255u),
                  DequantizeUNorm((value >> 16u) & 0xFFu, 255u),
                  DequantizeUNorm(value >> 24u, 255u));
}

BE_INLINE uint ShaderPacking::PackFloat4ToRGB10A2UNorm(const float4 &value) {
    return QuantizeUNorm(value.x, 1023u) |
          (QuantizeUNorm(value.y, 1023u) << 10u) |
          (QuantizeUNorm(value.z, 1023u) << 20u) |
          (QuantizeUNorm(value.w, 3u) << 30u);
}

BE_INLINE float4 ShaderPacking::UnpackRGB10A2UNormToFloat4(uint value) {
    return float4(DequantizeUNorm(value & 0x3FFu, 1023u),
                  DequantizeUNorm((value >> 10u) & 0x3FFu, 1023u),
                  DequantizeUNorm((value >> 20u) & 0x3FFu, 1023u),
                  DequantizeUNorm(value >> 30u, 3u));
}

BE_INLINE float3 ShaderPacking::UnpackRGB16FToFloat3(const uint2 &value) {
    const float2 xy = UnpackRG16FToFloat2(value.x);
    return float3(xy.x, xy.y, HalfBitsToFloat(value.y));
}

BE_INLINE float4 ShaderPacking::UnpackRGBA16FToFloat4(const uint2 &value) {
    const float2 xy = UnpackRG16FToFloat2(value.x);
    const float2 zw = UnpackRG16FToFloat2(value.y);
    return float4(xy.x, xy.y, zw.x, zw.y);
}

BE_INLINE float4 ShaderPacking::UnpackRGBA16UNormToFloat4(const uint2 &value) {
    const float2 xy = UnpackRG16UNormToFloat2(value.x);
    const float2 zw = UnpackRG16UNormToFloat2(value.y);
    return float4(xy.x, xy.y, zw.x, zw.y);
}
