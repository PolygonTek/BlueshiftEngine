shader "Lighting/Generic" {
	lighting
	properties {
		_DIFFUSE_SOURCE("Diffuse") : enum "Color;Texture" = "0" (shaderDefine)
		diffuseColor("Diffuse Color") : vec4 = "1 1 1 1"
		diffuseMap("Diffuse Map") : object TextureAsset = "_whiteTexture"
		_NORMAL_SOURCE("Normal") : enum "Vertex;Texture;Texture + Detail Texture" = "0" (shaderDefine)
		normalMap("Normal Map") : object TextureAsset = "_flatNormalTexture"
		detailNormalMap("Detail Normal Map") : object TextureAsset = "_flatNormalTexture"
		detailRepeat("Detail Repeat") : float = "8"
		_HALF_LAMBERT_DIFFUSE("Half Lambert Diffuse") : bool = "false" (shaderDefine)
		_SPECULAR_SOURCE("Specular") : enum "None;Color;Texture(RGB);Texture(RGB) + Exp(A);Diffuse Texture(A)" = "0" (shaderDefine)
		specularColor("Specular Color") : vec4 = "1 1 1 1"
		specularMap("Specular Map") : object TextureAsset = "_whiteTexture"
		specularExponent("Specular Exponent") : float range 0 256 1 = "16"
	}

	generatePerforatedVersion
	generateGpuSkinningVersion
	generateParallelShadowVersion
	generateSpotShadowVersion
	generatePointShadowVersion
	ambientLitVersion "amblit.shader"

	glsl_vp {
		$include "lightingGeneric.vp"
	}
	glsl_fp {
		$include "lightingGeneric.fp"
	}
}