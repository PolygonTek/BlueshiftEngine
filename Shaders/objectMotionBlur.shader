shader "motionBlurObject" {
	generatePerforatedVersion
	generateGpuSkinningVersion
    generateGpuInstancingVersion

	glsl_vp {
		$include "motionBlurObject.vp" 
	}
	glsl_fp {
		$include "motionBlurObject.fp" 
	}
}