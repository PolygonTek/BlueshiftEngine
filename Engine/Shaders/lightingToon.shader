shader "Lighting/Toon" {
	lighting
	properties {
		_DIFFUSE_SOURCE("Diffuse") : enum "Color;Map" = "0" (shaderDefine)
		_SPECULAR_SOURCE_SOURCE("Specular") : enum "None;Color;Map(RGB);" = "0" (shaderDefine)
	}

	generatePerforatedVersion
	generateGpuSkinningVersion
	generateParallelShadowVersion
	generateSpotShadowVersion
	generatePointShadowVersion
	ambientLitVersion "amblit"

	glsl_vp {
		$include "lightingToon.vp"
	}
	glsl_fp {
		$include "lightingToon.fp"
	}
}