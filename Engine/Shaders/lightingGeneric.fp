$include "fragment_common.glsl"

#ifdef USE_SHADOW_MAP
$include "shadow.fp"
#endif

$include "PhongLighting.glsl"
$include "BRDF.glsl"

in vec4 v2f_color;
in vec2 v2f_tcDiffuseNormal;
in vec2 v2f_tcSpecular;
in vec3 v2f_viewVector;
in vec3 v2f_lightVector;
in vec3 v2f_lightFallOff;
in vec4 v2f_lightProjection;
in vec3 v2f_normal;
in vec3 v2f_tangentToWorldMatrixS;
in vec4 v2f_tangentToWorldMatrixT;
//in vec3 v2f_tangentToWorldMatrixR;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D normalMap;
uniform sampler2D detailNormalMap;
uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D heightMap;
uniform sampler2D lightProjectionMap;
uniform sampler2D selfIllumMap;
uniform samplerCube lightCubeMap;

uniform vec4 diffuseColor;
uniform vec4 specularColor;
uniform vec4 lightColor;
uniform float detailRepeat;
uniform float lightFallOffExponent;
uniform bool useLightCube;
uniform bool useShadowMap;
uniform bool removeBackProjection;

uniform float roughness;
uniform float metalness;

uniform float reflectness;
uniform samplerCube envCubeMap;
uniform sampler2D envMaskMap;

uniform float rimLightExponent;
uniform float rimLightShadowDensity;// = 0.5;
uniform float subSurfaceRollOff;
uniform float subSurfaceShadowDensity;// = 0.5;
uniform sampler2D subSurfaceColorMap;

void main() {
    vec3 L = normalize(v2f_lightVector);

    float A = 1.0 - min(dot(v2f_lightFallOff, v2f_lightFallOff), 1.0);
    A = pow(A, lightFallOffExponent);

    vec3 Cl = tex2Dproj(lightProjectionMap, v2f_lightProjection).xyz * lightColor.xyz * A;

#if !defined(USE_SHADOW_POINT)
    if (removeBackProjection) {
        Cl *= min((v2f_lightProjection.z < 0.0 ? 0.0 : 10.0) * v2f_lightProjection.z / v2f_lightProjection.w, 1.0);
    }
#endif

    if (Cl == vec3(0.0)) {
        discard;
    }

    vec3 shadowLighting = vec3(1.0);

#if defined(USE_SHADOW_MAP)
#if defined(USE_SHADOW_SPOT) || defined(USE_SHADOW_POINT) || defined(USE_SHADOW_CASCADE)
    shadowLighting = ShadowFunc();
    /*#if !defined(RIM_LIGHT) && !defined(SUB_SURFACE_SCATTERING)
    if (shadowLighting == 0.0) {
        discard;
    }
    #endif*/
#endif
#endif

    vec3 V = normalize(v2f_viewVector);

#ifdef _PARALLAX
    vec2 tc = offsetTexcoord(heightMap, v2f_tcDiffuseNormal, V.xy, 0.0078125);
#else
    vec2 tc = v2f_tcDiffuseNormal;
#endif

#if _DIFFUSE_SOURCE == 0
    vec4 Kd = diffuseColor;
#elif _DIFFUSE_SOURCE == 1
    vec4 Kd = tex2D(diffuseMap, tc);
#endif

#ifdef PERFORATED
    if (Kd.w < 0.5) {
        discard;
    }
#endif

#if _NORMAL_SOURCE == 0
    vec3 N = normalize(v2f_normal);
#elif _NORMAL_SOURCE == 1
    vec3 N = normalize(getNormal(normalMap, tc));
#elif _NORMAL_SOURCE == 2
    vec3 b1 = normalize(getNormal(normalMap, tc));
    vec3 b2 = vec3(tex2D(detailNormalMap, tc * detailRepeat).xy * 2.0 - 1.0, 0.0);
    vec3 N = normalize(b1 + b2);
#endif

#if _SPECULAR_SOURCE == 0
    vec4 Ks = vec4(0.0, 0.0, 0.0, 0.0);
#elif _SPECULAR_SOURCE == 1
    vec4 Ks = specularColor;
#elif (_SPECULAR_SOURCE == 2 || _SPECULAR_SOURCE == 3)
    vec4 Ks = tex2D(specularMap, v2f_tcSpecular);
#elif _SPECULAR_SOURCE == 4
    vec4 Ks = tex2D(diffuseMap, v2f_tcSpecular).aaaa;
#endif

    vec3 Cd, Cs;
    litBlinnPhongEC(L, N, V, Kd, Ks, Cd, Cs);
    //litStandard(L, N, V, Kd.rgb, roughness, metalness, Cd, Cs);

    vec3 C = Cd;

    C += Cs;

#if defined(_RIM_LIGHT)
    float NdotV = max(dot(N, V), 0.0);
    float rimLight = max(pow(1.0 - NdotV, rimLightExponent) * max(dot(V, -L), 0.0), 0.0);
    C += rimLight * (shadowLighting * (1.0 - rimLightShadowDensity) + rimLightShadowDensity);
#endif

#if defined(_SUB_SURFACE_SCATTERING)
    float subLamb = smoothstep(-subSurfaceRollOff, 1.0, NdotL) - smoothstep(0.0, 1.0, NdotL);
    subLamb = max(subLamb, 0.0);
    C += subLamb * tex2D(subSurfaceColorMap, tc).xyz * (shadowLighting * (1.0 - subSurfaceShadowDensity) + subSurfaceShadowDensity);
#endif

    C *= shadowLighting;

#ifdef USE_LIGHT_CUBE_MAP
    if (useLightCube) {
        Cl *= texCUBE(lightCubeMap, -L);
    }
#endif
    vec4 outputColor = v2f_color * vec4(Cl * C, 1.0);

#ifdef LOGLUV_HDR
    o_fragColor = encodeLogLuv(outputColor.xyz);
#else
    o_fragColor = outputColor;
#endif
}
