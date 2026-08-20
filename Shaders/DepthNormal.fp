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
    in HIGHP vec3 v2f_normalWS;
    in HIGHP vec4 v2f_tangentWS;
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
    HIGHP vec3 surfaceNormalWS = normalize(v2f_normalWS);
    HIGHP vec3 tangentWS = v2f_tangentWS.xyz - surfaceNormalWS * dot(surfaceNormalWS, v2f_tangentWS.xyz);
    HIGHP float tangentLengthSquared = dot(tangentWS, tangentWS);
    tangentWS = tangentLengthSquared > 1e-20 ? tangentWS * inversesqrt(tangentLengthSquared) : vec3(1.0, 0.0, 0.0);
    HIGHP vec3 bitangentWS = cross(surfaceNormalWS, tangentWS) * v2f_tangentWS.w;

    vec3 normalTS = normalize(GetNormal(normalMap, baseTc));
    vec3 normalWS = mat3(tangentWS, bitangentWS, surfaceNormalWS) * normalTS;
#else
    vec3 normalWS = normalize(v2f_normalWS.xyz);
#endif

    o_fragNormal = vec4(normalWS, 1.0);
}
