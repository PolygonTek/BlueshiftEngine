$include "FragmentCommon.glsl"

#if defined(PERFORATED) || _NORMAL != 0
    #ifdef NEED_BASE_TC
#endif

out vec4 o_fragColor    : FRAG_COLOR0;
out vec4 o_fragNormal   : FRAG_COLOR1;

uniform sampler2D albedoMap;
uniform LOWP float perforatedAlpha;
uniform LOWP vec4 constantColor;

in VS_OUT {
#ifdef NEED_BASE_TC
    MEDIUMP vec2 tex;
#endif

#if _NORMAL != 0
    LOWP vec3 tangentToWorldMatrixS;
    LOWP vec3 tangentToWorldMatrixT;
    LOWP vec3 tangentToWorldMatrixR;
#else
    LOWP vec3 normalWS;
#endif

#if _PARALLAX
    vec3 viewTS;
#endif
} fs_in;

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
        vec2 baseTc = ParallaxMapping(heightMap, fs_in.tex, heightScale, normalize(fs_in.viewTS));
    #else
        vec2 baseTc = fs_in.tex;
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
    vec3 normalWS = mat3(normalize(fs_in.tangentToWorldMatrixS), normalize(fs_in.tangentToWorldMatrixT), normalize(fs_in.tangentToWorldMatrixR)) * normalTS;
#else
    vec3 normalWS = normalize(fs_in.normalWS.xyz);
#endif

    o_fragNormal = vec4(normalWS, 1.0);
}
