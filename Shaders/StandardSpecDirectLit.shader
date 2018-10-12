shader "Lit/StandardSpecDirectLit" {
    litSurface
    inheritProperties "StandardSpec"
    
    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion
    generateGpuInstancingVersion
    generateParallelShadowVersion
    generateSpotShadowVersion
    generatePointShadowVersion

    glsl_vp {
        #define STANDARD_SPECULAR_LIGHTING
        #define DIRECT_LIGHTING
        $include "StandardCore.vp"
    }
    glsl_fp {
        #define STANDARD_SPECULAR_LIGHTING
        #define DIRECT_LIGHTING
        $include "StandardCore.fp"
    }
}
