shader "Lit/StandardAmbientLitDirectLit" {
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
        #define INDIRECT_LIGHTING 1
        #define DIRECT_LIGHTING 1
        $include "StandardCore.vp"
    }
    glsl_fp {
        #define STANDARD_METALLIC_LIGHTING
        #define INDIRECT_LIGHTING 1
        #define DIRECT_LIGHTING 1
        $include "StandardCore.fp"
    }
}
