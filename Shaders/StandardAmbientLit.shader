shader "Lit/StandardAmbientLit" {
    litSurface
    inheritProperties "Standard"

    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion
    generateGpuInstancingVersion

    glsl_vp {
        #define STANDARD_METALLIC_LIGHTING
        #define INDIRECT_LIGHTING
        $include "StandardCore.vp"
    }
    glsl_fp {
        #define STANDARD_METALLIC_LIGHTING
        #define INDIRECT_LIGHTING
        $include "StandardCore.fp"
    }
}
