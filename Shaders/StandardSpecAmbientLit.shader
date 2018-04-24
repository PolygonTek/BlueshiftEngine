shader "Lit/StandardSpecAmbientLit" {
    litSurface
    inheritProperties "StandardSpec"

    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion
    generateGpuInstancingVersion

    glsl_vp {
        #define STANDARD_SPECULAR_LIGHTING
        #define INDIRECT_LIGHTING
        $include "StandardCore.vp"
    }
    glsl_fp {
        #define STANDARD_SPECULAR_LIGHTING
        #define INDIRECT_LIGHTING
        $include "StandardCore.fp"
    }
}
