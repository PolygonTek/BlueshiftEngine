shader "Lit/StandardSpecAmbientLit" {
    litSurface
    inheritProperties "StandardSpec.shader"

    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion

    glsl_vp {
        #define STANDARD_SPECULAR_LIGHTING
        #define INDIRECT_LIGHTING 1
        $include "StandardCore.vp"
    }
    glsl_fp {
        #define STANDARD_SPECULAR_LIGHTING
        #define INDIRECT_LIGHTING 1
        $include "StandardCore.fp"
    }
}
