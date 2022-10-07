shader "blendLight" {
	generateGpuSkinningVersion
    generateGpuInstancingVersion

	glsl_vp {
		$include "blendlight.vp" 
	}
	glsl_fp {
		$include "blendlight.fp" 
	}
}