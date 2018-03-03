shader "motionBlurObject" {
	generatePerforatedVersion
	generateGpuSkinningVersion
	
	glsl_vp {
		$include "motionBlurObject.vp" 
	}
	glsl_fp {
		$include "motionBlurObject.fp" 
	}
}