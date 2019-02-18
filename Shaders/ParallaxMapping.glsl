vec2 ParallaxMapping(sampler2D heightMap, vec2 texCoords, float heightScale, vec3 viewTS) {
#ifdef GL_ES
    float height = tex2D(heightMap, texCoords).x * 2.0 - 1.0;
    vec2 p = height * heightScale * viewTS.xy / viewTS.z;
    return texCoords + p;
#else
    const float minLayers = 8;
    const float maxLayers = 20;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewTS)));

    float layerDepth = 1.0 / numLayers;

    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 p = heightScale * viewTS.xy / viewTS.z;
    vec2 deltaTexCoords = p / numLayers;

    vec2 currentTexCoords = texCoords;

    float currentHeight = tex2D(heightMap, currentTexCoords).x * 2.0 - 1.0;
    float currentLayerDepth = -1.0;
    
    while (currentHeight >= currentLayerDepth) {
        // shift texture coordinates along direction of P
        currentTexCoords += deltaTexCoords * currentHeight;
        // get heightmap value at current texture coordinates
        currentHeight = tex2D(heightMap, currentTexCoords).x * 2.0 - 1.0;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    return currentTexCoords;
#endif
}
