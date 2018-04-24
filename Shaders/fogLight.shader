shader "fogLight" {
	generateGpuSkinningVersion
    generateGpuInstancingVersion

	glsl_vp {
		$include "foglight.vp" 
	}
	glsl_fp {
		$include "foglight.fp" 
	}
}