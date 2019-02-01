#ifndef SHADING_PARMS_INCLUDED
#define SHADING_PARMS_INCLUDED

struct ShadingParms {
    vec4 diffuse;
    vec4 specular;
    float roughness;
    float linearRoughness;

    vec3 v; // view vector in world space
    vec3 l; // light vector in world space
    vec3 n; // normal vector in world space
    float ndotv;
    vec2 dfg;
    vec3 energyCompensation;

    HIGHP mat3 tagentToWorldMatrix;

#ifdef LEGACY_PHONG_LIGHTING
    float specularPower;
#endif

#if _ANISO != 0
    vec3 anisotropicT;
    vec3 anisotropicB;
    float anisotropy;
#endif

#if _CLEARCOAT != 0
    float clearCoat;
    float clearCoatRoughness;
    float clearCoatLinearRoughness;
    vec3 clearCoatN;
#endif

#if _OCC != 0
    float occlusion;
#endif

#if _EMISSION != 0
    vec3 emission;
#endif
};

ShadingParms shading;

#endif
