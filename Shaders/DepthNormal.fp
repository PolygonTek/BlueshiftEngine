$include "FragmentCommon.glsl"

#if defined(PERFORATED) || _NORMAL != 0
    #ifdef NEED_BASE_TC
#endif

out vec4 o_fragColor    : FRAG_COLOR0;
out vec4 o_fragNormal   : FRAG_COLOR1;

uniform sampler2D albedoMap;
uniform LOWP float perforatedAlpha;
uniform LOWP vec4 constantColor;

#ifdef NEED_BASE_TC
    in MEDIUMP vec2 v2f_tex;
#endif

#if _NORMAL != 0
    in LOWP vec3 v2f_tangentToWorldMatrixS;
    in LOWP vec3 v2f_tangentToWorldMatrixT;
    in LOWP vec3 v2f_tangentToWorldMatrixR;
#else
    in LOWP vec3 v2f_normalWS;
#endif

#if _PARALLAX
    in HIGHP vec3 v2f_viewTS;
#endif

#if _NORMAL != 0
    uniform sampler2D normalMap;
#endif

#if _PARALLAX != 0
    uniform sampler2D heightMap;
    uniform LOWP float heightScale;
#endif

#if _PARALLAX != 0
    $include "ParallaxMapping.glsl"
#endif

void main() {
#ifdef NEED_BASE_TC
    #if _PARALLAX != 0
        vec2 baseTc = ParallaxMapping(heightMap, v2f_tex, heightScale, normalize(v2f_viewTS));
    #else
        vec2 baseTc = v2f_tex;
    #endif
#endif

#ifdef PERFORATED
    vec4 albedo = tex2D(albedoMap, baseTc);
    if (albedo.a < perforatedAlpha) {
        discard;
    }
#endif

    o_fragColor = constantColor;

#if _NORMAL != 0
    vec3 normalTS = normalize(GetNormal(normalMap, baseTc));
    vec3 normalWS = mat3(normalize(v2f_tangentToWorldMatrixS), normalize(v2f_tangentToWorldMatrixT), normalize(v2f_tangentToWorldMatrixR)) * normalTS;
#else
    vec3 normalWS = normalize(v2f_normalWS.xyz);
#endif

    o_fragNormal = vec4(normalWS, 1.0);
}
