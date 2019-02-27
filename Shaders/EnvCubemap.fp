in MEDIUMP vec3 v2f_texCoord;

out vec4 o_fragColor : FRAG_COLOR;

uniform MEDIUMP float mipLevel;

uniform HIGHP samplerCube envCubeMap;

void main() {
#if _EXPLICIT_MIPMAPS == 1
    vec3 color = texCUBElod(envCubeMap, vec4(v2f_texCoord, mipLevel)).rgb;
#else
    vec3 color = texCUBE(envCubeMap, v2f_texCoord).rgb;
#endif

    o_fragColor = vec4(color, 1.0);
}
