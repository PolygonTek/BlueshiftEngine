shader "Lit/StandardDirectLit" {
    litSurface
    inheritProperties "Standard.shader"
    
    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion
    generateParallelShadowVersion
    generateSpotShadowVersion
    generatePointShadowVersion

    glsl_vp {
        #define STANDARD_METALLIC_LIGHTING
        #define DIRECT_LIGHTING
        $include "StandardCore.vp"
    }
    glsl_fp {
        #define STANDARD_METALLIC_LIGHTING
        #define DIRECT_LIGHTING
        $include "StandardCore.fp"
    }
}
