shader "amblit" {
	properties {
		_DIFFUSE_SOURCE("Diffuse") : enum "Color;Texture" = "0" (shaderDefine)
		diffuseColor("Diffuse Color") : vec4 = "1 1 1 1"
		diffuseMap("Diffuse Map") : object TextureAsset = "_whiteTexture"
		_NORMAL_SOURCE("Normal") : enum "Vertex;Texture;Texture + Detail Texture" = "0" (shaderDefine)
		normalMap("Normal Map") : object TextureAsset = "_flatNormalTexture"
		detailNormalMap("Detail Normal Map") : object TextureAsset = "_flatNormalTexture"
		detailRepeat("Detail Repeat") : float = "8"
	}
	
	generatePerforatedVersion
	generateGpuSkinningVersion

	glsl_vp {
		$include "amblit.vp"
	}
	glsl_fp {
		$include "amblit.fp"
	}
}
