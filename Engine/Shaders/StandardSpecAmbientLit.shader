shader "Lit/StandardSpecAmbientLit" {
    litSurface
    inheritProperties "StandardSpec.shader"

    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion

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
