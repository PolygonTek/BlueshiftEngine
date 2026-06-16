#include "Common.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(UAV(u0)), " \
    "CBV(b0), " \
    "RootConstants(num32BitConstants=12, b1)"

cbuffer FrameCB : register(b0) {
    FrameConstants frameConstants;
};

cbuffer PostProcessCB : register(b1) {
    PostProcessParams postProcessParams;
}

RWByteAddressBuffer luminanceHistogram : register(u0);

// local histogram bin table
groupshared uint histogram[LuminanceHistogramBinCount];

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(LuminanceBlockSize, LuminanceBlockSize, 1)]
void CSMain(uint binIndex : SV_GroupIndex) {
    // binIndex 가 곧 양자화된 histogram 값이니까 개수를 곱해서 histogram 의 bin 별 누적값을 구한다.
    float countForThisBin = (float)luminanceHistogram.Load(LuminanceBufferOffsetHistogram + binIndex * 4);
    histogram[binIndex] = countForThisBin * (float)binIndex;

    GroupMemoryBarrierWithGroupSync();

    // histogram 을 reduce 해서 histogram[0] 에 모든 합을 계산한다.
    [unroll]
    for (uint histogramSampleIndex = (LuminanceHistogramBinCount >> 1); histogramSampleIndex > 0; histogramSampleIndex >>= 1) {
        if (binIndex < histogramSampleIndex) {
            histogram[binIndex] += histogram[binIndex + histogramSampleIndex];
        }

        GroupMemoryBarrierWithGroupSync();
    }

    if (binIndex == 0) {
        // histogram[0] 를 제외한 luminance 의 log 평균값을 구한다.
        float weightedLogAverage = (histogram[0].x / max(PPP_LUMINANCE_PIXEL_COUNT - countForThisBin, 1.0)) - 1.0;

        // luminance 평균값으로 변환한다.
        float weightedAverageLuminance = exp2(((weightedLogAverage / (LuminanceHistogramBinCount - 2)) * PPP_LUMINANCE_LOG_RANGE) + PPP_LUMINANCE_LOG_MIN);
        
        // 이전 프레임의 luminance 값과 현재의 평균값을 exp smoothing 으로 보간한다.
        float luminanceLastFrame = luminanceHistogram.Load<float>(LuminanceBufferOffsetLuminance);
        float adaptationFrameTime = max(frameConstants.frameTime, 0.0);
        float adaptedLuminance = luminanceLastFrame + (weightedAverageLuminance - luminanceLastFrame) * (1 - exp(-adaptationFrameTime * PPP_LUMINANCE_ADAPTATION_RATE));

        // 보간된 luminance 값 저장 (다음 프레임에 사용)
        luminanceHistogram.Store<float>(LuminanceBufferOffsetLuminance, adaptedLuminance);

        // exposure 값 저장
        luminanceHistogram.Store<float>(LuminanceBufferOffsetExposure, PPP_LUMINANCE_MIDDLE_GRAY / max(adaptedLuminance, 0.0001));
    }

    // clear histogram for next frame
    luminanceHistogram.Store(LuminanceBufferOffsetHistogram + binIndex * 4, 0);
}
