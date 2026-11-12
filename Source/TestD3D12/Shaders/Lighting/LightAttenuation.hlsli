#ifndef __LIGHT_ATTENUATION_HLSLI__
#define __LIGHT_ATTENUATION_HLSLI__

// 광원 거리 감쇠를 계산한다. (역제곱 감쇠 + 반경 경계에서 부드럽게 0 이 되는 범위 감쇠)
float CalcLightDistanceAttenuation(float lightDistanceSquared, float invRadiusSquared, float distanceBiasSquared = 0.0f) {
    // 역제곱 감쇠에 반경 경계에서 부드럽게 0 이 되는 범위 감쇠를 적용한다.
    const float normalizedDistanceSquared = lightDistanceSquared * invRadiusSquared;
    const float rangeFade = saturate(1.0f - normalizedDistanceSquared * normalizedDistanceSquared);

    // 거리 바이어스로 광원 중심의 피크를 완화한다. 기본값 0 은 기존 표면 조명을 유지한다.
    // 1cm 이내의 역제곱 감쇠는 고정하여 발산을 제한한다.
    const float minDistanceSquared = 0.01f * 0.01f;
    return (rangeFade * rangeFade) / max(lightDistanceSquared + distanceBiasSquared, minDistanceSquared);
}

// Spot 라이트의 각도 감쇠를 계산한다. (inner 안쪽은 1, outer 바깥은 0)
float CalcSpotLightAngularAttenuation(float cosTheta, float cosOuterCone, float invCosConeDifference) {
    // Inner 안쪽은 1, outer 바깥은 0. CPU 에서 준비한 계수로 픽셀당 삼각함수/나눗셈을 피한다.
    const float coneFade = saturate((cosTheta - cosOuterCone) * invCosConeDifference);
    return coneFade * coneFade;
}

#endif
