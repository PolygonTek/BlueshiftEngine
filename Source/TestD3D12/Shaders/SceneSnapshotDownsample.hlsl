#define SCENE_SNAPSHOT_DOWNSAMPLE_ROOT_SIGNATURE \
    "DescriptorTable(SRV(t0, numDescriptors = 2))"

struct PSInput {
    float4 position : SV_POSITION;
};

struct PSOutput {
    float4 color : SV_TARGET0;
    float depth : SV_TARGET1;
};

Texture2D<float4> sourceColorTexture : register(t0);
Texture2D<float> sourceDepthTexture : register(t1);

[RootSignature(SCENE_SNAPSHOT_DOWNSAMPLE_ROOT_SIGNATURE)]
PSInput VSMain(uint vertexID : SV_VertexID) {
    PSInput result;
    result.position.x = float(vertexID % 2u) * 4.0f - 1.0f;
    result.position.y = float(vertexID / 2u) * 4.0f - 1.0f;
    result.position.z = 0.0f;
    result.position.w = 1.0f;
    return result;
}

[RootSignature(SCENE_SNAPSHOT_DOWNSAMPLE_ROOT_SIGNATURE)]
PSOutput PSMain(PSInput input) {
    uint sourceWidth;
    uint sourceHeight;
    sourceColorTexture.GetDimensions(sourceWidth, sourceHeight);

    // 각 2x2 영역에서 같은 원본 texel 의 color 와 device depth 를 보존한다.
    const uint2 outputCoord = uint2(input.position.xy);
    const uint2 sourceCoord = min(outputCoord * 2u + 1u, uint2(sourceWidth - 1u, sourceHeight - 1u));

    PSOutput result;
    result.color = sourceColorTexture.Load(int3(sourceCoord, 0));
    result.depth = sourceDepthTexture.Load(int3(sourceCoord, 0));
    return result;
}
