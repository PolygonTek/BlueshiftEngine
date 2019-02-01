#ifndef STANDARD_CONFIG_INCLUDED
#define STANDARD_CONFIG_INCLUDED

#define PBR_DIFFUSE_LAMBERT             0
#define PBR_DIFFUSE_WRAPPED             1
#define PBR_DIFFUSE_BURLEY              2
#define PBR_DIFFUSE_OREN_NAYAR          3

#define PBR_SPECULAR_D_BLINN            0
#define PBR_SPECULAR_D_BECKMANN         1
#define PBR_SPECULAR_D_GGX              2

#define PBR_SPECULAR_G_NEUMANN          0
#define PBR_SPECULAR_G_KELEMEN          1
#define PBR_SPECULAR_G_COOK_TORRANCE    2
#define PBR_SPECULAR_G_SCHLICK_GGX      3
#define PBR_SPECULAR_G_SMITH_GGX        4
#define PBR_SPECULAR_G_SMITH_GGX_FAST   5

#ifdef GL_ES
#define PBR_DIFFUSE         PBR_DIFFUSE_LAMBERT
#define PBR_SPECULAR_D      PBR_SPECULAR_D_GGX
#define PBR_SPECULAR_G      PBR_SPECULAR_G_SCHLICK_GGX
#else
#define PBR_DIFFUSE         PBR_DIFFUSE_BURLEY
#define PBR_SPECULAR_D      PBR_SPECULAR_D_GGX
#define PBR_SPECULAR_G      PBR_SPECULAR_G_SCHLICK_GGX
#endif

#define PBR_CLEARCOAT_D     PBR_SPECULAR_D_GGX
#define PBR_CLEARCOAT_G     PBR_SPECULAR_G_KELEMEN

#define USE_BLINN_PHONG

#define USE_MULTIPLE_SCATTERING_COMPENSATION

//#define PARALLAX_CORRECTED_INDIRECT_LIGHTING

#define ENABLE_DETAIL_NORMALMAP
#define ENABLE_PARALLAXMAP

#ifndef _ALBEDO
#define _ALBEDO 0
#endif

#ifndef _NORMAL
#define _NORMAL 0
#endif

#ifndef _SPECULAR
#define _SPECULAR 0
#endif

#ifndef _GLOSS
#define _GLOSS 0
#endif

#ifndef _METALLIC
#define _METALLIC 0
#endif

#ifndef _ROUGHNESS
#define _ROUGHNESS 0
#endif

#ifndef _ANISO
#define _ANISO 0
#endif

#ifndef _CLEARCOAT
#define _CLEARCOAT 0
#endif

#ifndef _CC_ROUGHNESS
#define _CC_ROUGHNESS 0
#endif

#ifndef _CC_NORMAL
#define _CC_NORMAL 0
#endif

#ifndef _PARALLAX
#define _PARALLAX 0
#endif

#ifndef _EMISSION
#define _EMISSION 0
#endif

#if _NORMAL == 2 && !defined(ENABLE_DETAIL_NORMALMAP)
    #undef _NORMAL
    #define _NORMAL 1
#endif

#if _PARALLAX == 1 && !defined(ENABLE_PARALLAXMAP)
    #undef _PARALLAX 
    #define _PARALLAX 0
#endif

#endif
