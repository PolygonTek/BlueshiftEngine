shader "fogLight" {
	generateGpuSkinningVersion
	
	glsl_vp {
		$include "foglight.vp" 
	}
	glsl_fp {
		$include "foglight.fp" 
	}
}