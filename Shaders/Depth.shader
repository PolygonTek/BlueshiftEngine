shader "Depth" {
    generatePerforatedVersion
    generateGpuSkinningVersion
    generateGpuInstancingVersion
    
    glsl_vp {
        $include "Depth.vp"
    }
    glsl_fp {
        $include "Depth.fp"
    }
}