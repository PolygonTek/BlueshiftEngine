shader "Lit/PhongDirectLit" {
    litSurface
    inheritProperties "Phong"
    
    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion
    generateGpuInstancingVersion
    generateParallelShadowVersion
    generateSpotShadowVersion
    generatePointShadowVersion

    glsl_vp {
        #define LEGACY_PHONG_LIGHTING
        #define DIRECT_LIGHTING
        $include "StandardCore.vp"
    }
    glsl_fp {
        #define LEGACY_PHONG_LIGHTING
        #define DIRECT_LIGHTING
        $include "StandardCore.fp"
    }
}
