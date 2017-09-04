in LOWP vec4 v2f_color;
in MEDIUMP vec3 v2f_texCoord;

out vec4 o_fragColor : FRAG_COLOR;
out float o_fragDepth : FRAG_DEPTH;

uniform LOWP vec4 tint;
uniform MEDIUMP float exposure;

uniform HIGHP samplerCube skyCubeMap;

void main() {
    vec3 color = texCUBE(skyCubeMap, v2f_texCoord).rgb;
    color *= tint.rgb * exposure;

    o_fragColor = v2f_color * vec4(color, 1.0);
    o_fragDepth = 1.0;
}
