shader "SelectionId" {
	generatePerforatedVersion
	generateGpuSkinningVersion

	glsl_vp {
		$include "SelectionId.vp"
	}
	glsl_fp {
		$include "SelectionId.fp"
	}
}
