shader "liquid" {
	properties {
		_INTERPOLATE("Interpolate") : bool = "true" (shaderDefine)
		_SUN "Sun" : bool = "true" (shaderDefine)
		sunColor("Sun Color") : color = "1 1 1"
		sunDirection("Sun Direction") : vec3 = "0 0 1"
		sunSpecularExponent("Sun Specular Exponent") : float range 0 256 1 = "16"
		fresnelConstant("Fresnel Constant") : float = "1.3333"
	}

	glsl_vp {
		$include "liquid.vp" 
	}
	glsl_fp {
		$include "liquid.fp" 
	}	
}
