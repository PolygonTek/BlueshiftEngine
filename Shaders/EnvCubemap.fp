in LOWP vec3 v2f_viewWS;
in LOWP vec3 v2f_normal;

out vec4 o_fragColor : FRAG_COLOR;

uniform MEDIUMP float mipLevel;

uniform HIGHP samplerCube envCubeMap;

void main() {
    vec3 worldS = reflect(-normalize(v2f_viewWS), normalize(v2f_normal));

#if _EXPLICIT_MIPMAPS == 1
    vec3 color = texCUBElod(envCubeMap, vec4(worldS, mipLevel)).rgb;
#else
    //vec3 color = texCUBE(envCubeMap, worldS).rgb;
    vec3 color = texCUBElod(envCubeMap, vec4(worldS, 0)).rgb;
#endif

    o_fragColor = vec4(color, 1.0);
}
