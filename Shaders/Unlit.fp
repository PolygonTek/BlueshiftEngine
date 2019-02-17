#ifndef _ALBEDO
#define _ALBEDO 0
#endif

#if _ALBEDO != 0
    #define NEED_BASE_TC
#endif

in VS_OUT {
    LOWP vec4 color;

#ifdef NEED_BASE_TC
    MEDIUMP vec2 texCoord;
#endif
} fs_in;

out vec4 o_fragColor : FRAG_COLOR;

#if _ALBEDO == 0
    uniform LOWP vec3 albedoColor;
    uniform LOWP float albedoAlpha;
#elif _ALBEDO == 1
    uniform sampler2D albedoMap;
#endif

void main() {
#if _ALBEDO == 0
    vec4 albedo = vec4(albedoColor, albedoAlpha);
#elif _ALBEDO == 1
    vec4 albedo = tex2D(albedoMap, fs_in.texCoord);
#endif

#ifdef PERFORATED
    if (albedo.a < perforatedAlpha) {
        discard;
    }
#endif

    o_fragColor = fs_in.color * albedo;
}
