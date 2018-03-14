shader "Lit/StandardSpecAmbientLitDirectLit" {
    litSurface
    inheritProperties "StandardSpec.shader"
    
    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion
    generateParallelShadowVersion
    generateSpotShadowVersion
    generatePointShadowVersion

    glsl_vp {
        #define STANDARD_SPECULAR_LIGHTING
        #define INDIRECT_LIGHTING
        #define DIRECT_LIGHTING
        $include "StandardCore.vp"
    }
    glsl_fp {
        #define STANDARD_SPECULAR_LIGHTING
        #define INDIRECT_LIGHTING
        #define DIRECT_LIGHTING
        $include "StandardCore.fp"
    }
}
