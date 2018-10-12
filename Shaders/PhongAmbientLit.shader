shader "Lit/PhongAmbientLit" {
    litSurface
    inheritProperties "Phong"

    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion
    generateGpuInstancingVersion

    glsl_vp {
        #define LEGACY_PHONG_LIGHTING
        #define INDIRECT_LIGHTING
        $include "StandardCore.vp"
    }
    glsl_fp {
        #define LEGACY_PHONG_LIGHTING
        #define INDIRECT_LIGHTING
        $include "StandardCore.fp"
    }
}
