#ifndef _ALBEDO
#define _ALBEDO 0
#endif

#if _ALBEDO != 0
    #define NEED_BASE_TC
#endif

in LOWP vec4 v2f_color;

#ifdef NEED_BASE_TC
    in MEDIUMP vec2 v2f_texCoord;
#endif

out vec4 o_fragColor : FRAG_COLOR;

#if _ALBEDO == 0
    uniform LOWP vec3 albedoColor;
    uniform LOWP float albedoAlpha;
#elif _ALBEDO == 1
    uniform sampler2D albedoMap;
#endif

uniform MEDIUMP float intensity;

void main() {
#if _ALBEDO == 0
    vec4 albedo = vec4(albedoColor, albedoAlpha);
#elif _ALBEDO == 1
    vec4 albedo = tex2D(albedoMap, v2f_texCoord);
#endif

#ifdef PERFORATED
    if (albedo.a < perforatedAlpha) {
        discard;
    }
#endif

    o_fragColor = v2f_color * albedo * intensity;
}
