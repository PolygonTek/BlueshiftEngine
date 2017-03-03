shader "selectionId" {
	generatePerforatedVersion
	generateGpuSkinningVersion

	glsl_vp {
		$include "selectionId.vp"
	}
	glsl_fp {
		$include "selectionId.fp"
	}
}
