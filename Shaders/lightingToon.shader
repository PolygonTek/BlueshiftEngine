shader "Lighting/Toon" {
    litSurface
	properties {
		_ALBEDO("Diffuse") : enum "Color;Map" = "0" (shaderDefine)
		_SPECULAR_SOURCE_SOURCE("Specular") : enum "None;Color;Map(RGB);" = "0" (shaderDefine)
	}

	generatePerforatedVersion
	generateGpuSkinningVersion
	generateParallelShadowVersion
	generateSpotShadowVersion
	generatePointShadowVersion
	ambientLitVersion "ForwardBase"

	glsl_vp {
		$include "lightingToon.vp"
	}
	glsl_fp {
		$include "lightingToon.fp"
	}
}