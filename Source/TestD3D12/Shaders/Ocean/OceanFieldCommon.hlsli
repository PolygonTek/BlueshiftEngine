#ifndef __OCEAN_FIELD_COMMON_HLSLI__
#define __OCEAN_FIELD_COMMON_HLSLI__

float CalculateOceanJacobian(float Dxdy, float Dxdx, float Dydy) {
    return (1.0f + Dxdx) * (1.0f + Dydy) - Dxdy * Dxdy;
}

float3 CalculateOceanSurfaceNormal(float Dxdy, float4 derivativeField) {
    const float Dzdx = derivativeField.x;
    const float Dzdy = derivativeField.y;
    const float Dxdx = derivativeField.z;
    const float Dydy = derivativeField.w;
    const float J = CalculateOceanJacobian(Dxdy, Dxdx, Dydy);

    // tangent cross product
    const float3 normal = float3(
        Dzdy * Dxdy - Dzdx * (1.0f + Dydy),
        Dzdx * Dxdy - Dzdy * (1.0f + Dxdx),
        J);

    const float normalLengthSquared = dot(normal, normal);
    return normalLengthSquared > 1e-12f ? normal * rsqrt(normalLengthSquared) : float3(0.0f, 0.0f, 1.0f);
}

#endif
