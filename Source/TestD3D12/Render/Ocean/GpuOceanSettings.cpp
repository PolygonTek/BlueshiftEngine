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

#include "Precompiled.h"
#include "GpuOcean.h"

GpuOcean::SimulationSettings::SimulationSettings() {
    cascadeSettings[0] = { 60.0f, 0.64f, 2.5f, 1.0f };
    cascadeSettings[1] = { 60.0f, 0.65f, 2.6f, 2.0f };
    cascadeSettings[2] = { 45.0f, 0.72f, 3.1f, 3.0f };
    cascadeSettings[3] = { 30.0f, 0.82f, 0.0f, 4.0f };

    for (uint32_t cascadeIndex = 0; cascadeIndex < MaxCascadeCount; ++cascadeIndex) {
        cascades[cascadeIndex].randomSeed = 123 * (cascadeIndex + 1);
    }
    InitCascades(512.0f, 0.5f);
}

void GpuOcean::SimulationSettings::InitCascades(float largestPatchSize, float beta) {
    assert(cascadeCount >= 1 && cascadeCount <= MaxCascadeCount);
    cascadeBeta = beta;

    const float highFrequencyBoundaryBin = beta * 0.5f * static_cast<float>(GpuFFT::GetGpuFFTSizeValue(fftSize));
    const float patchSizeRatio = cascadeCount > 1 ? BE1::Math::Pow(highFrequencyBoundaryBin, 1.0f / static_cast<float>(cascadeCount - 1)) : 1.0f;
    const float inversePatchSizeRatio = 1.0f / patchSizeRatio;

    float patchSize = largestPatchSize;
    float minWaveNumber = BE1::Math::TwoPi / patchSize;

    for (uint32_t cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex) {
        const bool isLastCascade = cascadeIndex == cascadeCount - 1;
        const float maxWaveNumber = isLastCascade ? BE1::Math::BigNumber : highFrequencyBoundaryBin * BE1::Math::TwoPi / patchSize;

        // Patch 분할을 다시 계산할 때 사용자가 지정한 cascade 별 random seed 는 보존한다.
        CascadeDesc &cascadeDesc = cascades[cascadeIndex];
        cascadeDesc.patchSize = patchSize;
        cascadeDesc.minWaveNumber = minWaveNumber;
        cascadeDesc.maxWaveNumber = maxWaveNumber;

        minWaveNumber = maxWaveNumber;
        patchSize *= inversePatchSizeRatio;
    }
}

bool GpuOcean::SimulationSettings::HasSameSpectrumModelParameters(const SimulationSettings &rhs) const {
    if (spectrumModel != rhs.spectrumModel ||
        windDirection != rhs.windDirection || windSpeed != rhs.windSpeed ||
        gravity != rhs.gravity || smallWaveDamping != rhs.smallWaveDamping) {
        return false;
    }

    switch (spectrumModel) {
    case SpectrumModel::Phillips:
        return phillips == rhs.phillips;
    case SpectrumModel::TMA:
        return depth == rhs.depth && tma == rhs.tma;
    }
    return false;
}

GpuOcean::SimulationSettings::ChangeFlag GpuOcean::SimulationSettings::ClassifyChanges(const SimulationSettings &rhs) const {
    ChangeFlag changes = ChangeFlag::None;

    // FFT 크기 또는 cascade 개수 변경 : GPU 리소스 재생성 및 전체 상태 초기화
    if (fftSize != rhs.fftSize || cascadeCount != rhs.cascadeCount) {
        changes |= ChangeFlag::RecreateResources | ChangeFlag::RegenInitialSpectrum | ChangeFlag::ResetFoamHistory;
    }

    // cascade descriptor 변경 : 스펙트럼 재생성 및 foam 초기화
    for (uint32_t cascadeIndex = 0; cascadeIndex < rhs.cascadeCount; ++cascadeIndex) {
        if (cascades[cascadeIndex] != rhs.cascades[cascadeIndex]) {
            changes |= ChangeFlag::RegenInitialSpectrum | ChangeFlag::ResetFoamHistory;
            break;
        }
    }

    // 스펙트럼 파라미터 변경 : 초기 스펙트럼만 재생성
    if (!HasSameSpectrumModelParameters(rhs)) {
        changes |= ChangeFlag::RegenInitialSpectrum;
    }

    // Cross derivative 생성 변경 : 초기 스펙트럼과 foam history 는 유지하고 모든 cascade field 만 즉시 다시 계산한다.
    if (crossDerivativeEnabled != rhs.crossDerivativeEnabled) {
        changes |= ChangeFlag::RefreshCascadeOutputs;
    }

    // foam 생성에 영향을 주는 설정 변경 : foam history 초기화
    if (spectrumModel != rhs.spectrumModel || displacementScale != rhs.displacementScale || choppiness != rhs.choppiness || depth != rhs.depth || foamEnabled != rhs.foamEnabled) {
        changes |= ChangeFlag::ResetFoamHistory;
    }

    return changes;
}

bool GpuOcean::SimulationSettings::IsValid() const {
    if (cascadeCount < 1 || cascadeCount > MaxCascadeCount) {
        return false;
    }

    if (displacementScale < 0.0f || timeScale < 0.0f || windDirection.LengthSqr() <= 0.0f ||
        windSpeed <= 0.0f || gravity <= 0.0f || depth <= 0.0f || smallWaveDamping < 0.0f || choppiness < 0.0f) {
        return false;
    }

    switch (spectrumModel) {
    case SpectrumModel::Phillips:
        if (phillips.spectrumScale < 0.0f) {
            return false;
        }
        break;
    case SpectrumModel::TMA:
        if (tma.spectrumScale < 0.0f || tma.fetch <= 0.0f ||
            tma.peakEnhancement < 1.0f || tma.peakEnhancement > 7.0f ||
            tma.swell < 0.0f || tma.swell > 1.0f || tma.spread < 0.0f || tma.spread > 1.0f) {
            return false;
        }
        switch (tma.directionalSpreadModel) {
        case DirectionalSpreadModel::PositiveCosineSquared:
        case DirectionalSpreadModel::Hasselmann:
        case DirectionalSpreadModel::DonelanBanner:
        case DirectionalSpreadModel::Mitsuyasu:
            break;
        default:
            return false;
        }
        break;
    default:
        return false;
    }

    for (uint32_t cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex) {
        const CascadeDesc &cascadeDesc = cascades[cascadeIndex];
        if (cascadeDesc.patchSize <= 0.0f || cascadeDesc.minWaveNumber < 0.0f || cascadeDesc.maxWaveNumber <= cascadeDesc.minWaveNumber) {
            return false;
        }
    }

    for (uint32_t cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex) {
        const CascadeSettings &cascade = cascadeSettings[cascadeIndex];
        if (cascade.updateRate <= 0.0f || cascade.foamGenerationRate < 0.0f || cascade.foamHalfLife <= 0.0f) {
            return false;
        }
    }

    return true;
}
