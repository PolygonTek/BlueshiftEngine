#ifndef __FULLSCREEN_TRIANGLE_HLSLI__
#define __FULLSCREEN_TRIANGLE_HLSLI__

inline void GetFullscreenTriangleVertex(uint vertexID, out float4 position, out float2 texCoord) {
    // Creates a full screen CCW triangle from 3 vertices
    float2 corner = float2(vertexID % 2u, vertexID / 2u);

    position = float4(corner * 4.0 - 1.0, 0.0, 1.0);
    texCoord = float2(corner.x * 2.0, 1.0 - corner.y * 2.0);
}

#endif // __FULLSCREEN_TRIANGLE_HLSLI__
