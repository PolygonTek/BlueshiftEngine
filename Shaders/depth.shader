shader "depth" {
	generatePerforatedVersion
	generateGpuSkinningVersion

	glsl_vp {
		$include "depth.vp"
	}
	glsl_fp {
		$include "depth.fp"
	}
}