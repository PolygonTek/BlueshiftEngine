shader "Lit/PhongAmbientLit" {
    litSurface
    inheritProperties "Phong.shader"

    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion

    glsl_vp {
        #define LEGACY_PHONG_LIGHTING
        #define INDIRECT_LIGHTING 1
        $include "StandardCore.vp"
    }
    glsl_fp {
        #define LEGACY_PHONG_LIGHTING
        #define INDIRECT_LIGHTING 1
        $include "StandardCore.fp"
    }
}
