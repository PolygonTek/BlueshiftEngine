#ifndef _ALBEDO
#define _ALBEDO 0
#endif

#ifndef _EMISSION
#define _EMISSION 0
#endif

#if _ALBEDO != 0 || _EMISSION == 2
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

#if _EMISSION != 0
    uniform MEDIUMP float emissionScale;
    #if _EMISSION == 1
        uniform LOWP vec3 emissionColor;
    #elif _EMISSION == 2
        uniform sampler2D emissionMap;
    #endif
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

    vec3 shadingColor = albedo.rgb;

#if _EMISSION == 1
    vec3 emission = emissionColor * emissionScale;
#elif _EMISSION == 2
    vec3 emission = tex2D(emissionMap, fs_in.texCoord).rgb * emissionScale;
#endif

#if _EMISSION != 0
    shadingColor += emission;
#endif

    o_fragColor = fs_in.color * vec4(shadingColor, albedo.a);
}
