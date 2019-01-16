#ifndef STANDARD_CONFIG_INCLUDED
#define STANDARD_CONFIG_INCLUDED

#define PBR_DIFFUSE_LAMBERT         0
#define PBR_DIFFUSE_WRAPPED         1
#define PBR_DIFFUSE_BURLEY          2
#define PBR_DIFFUSE_OREN_NAYAR      3

#define PBR_SPEC_D_BLINN            0
#define PBR_SPEC_D_BECKMANN         1
#define PBR_SPEC_D_GGX              2
#define PBR_SPEC_D_GGX_ANISO        3

#define PBR_SPEC_G_NEUMANN          0
#define PBR_SPEC_G_KELEMEN          1
#define PBR_SPEC_G_COOK_TORRANCE    2
#define PBR_SPEC_G_GGX              3

#ifdef GL_ES
#define PBR_DIFFUSE PBR_DIFFUSE_LAMBERT
#else
#define PBR_DIFFUSE PBR_DIFFUSE_BURLEY
#endif

#define PBR_SPEC_D PBR_SPEC_D_GGX
#define PBR_SPEC_G PBR_SPEC_G_GGX

#define USE_BLINN_PHONG

#define ENABLE_DETAIL_NORMALMAP
#define ENABLE_PARALLAXMAP

#endif
