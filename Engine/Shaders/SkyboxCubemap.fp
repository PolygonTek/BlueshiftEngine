in vec4 v2f_color;
in vec3 v2f_texCoord;

out vec4 o_fragColor : FRAG_COLOR;
out float o_fragDepth : FRAG_DEPTH;

uniform vec4 tint;
uniform float exposure;

uniform samplerCube skyCubeMap;

void main() {
    vec3 color = texCUBE(skyCubeMap, v2f_texCoord).rgb;
    color *= tint.rgb * exposure;

    o_fragColor = v2f_color * vec4(color, 1.0);
    o_fragDepth = 1.0;
}
