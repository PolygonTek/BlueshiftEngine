shader "luminanceAdaptation" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		#ifdef LOGLUV_HDR
		$include "logluv.glsl"
		#endif
		uniform sampler2D currLuminanceMap;
		uniform sampler2D prevLuminanceMap;
		uniform float frameTime;

		void main() {
		#ifdef LOGLUV_HDR
			float currentLum = decodeLogLuv(tex2D(currLuminanceMap, vec2(0.0, 0.0))).x;
			float adaptedLum = decodeLogLuv(tex2D(prevLuminanceMap, vec2(0.0, 0.0))).x;
		#else
			float currentLum = tex2D(currLuminanceMap, vec2(0.0, 0.0)).x;
			float adaptedLum = tex2D(prevLuminanceMap, vec2(0.0, 0.0)).x;
		#endif

			adaptedLum = max(adaptedLum, 0.0);

			float newAdaptation = adaptedLum + (currentLum - adaptedLum) * (1.0 - pow(0.98, 60.0 * frameTime));

		#ifdef LOGLUV_HDR
			o_fragColor = encodeLogLuv(vec3(newAdaptation, newAdaptation, newAdaptation));
		#else
			o_fragColor = vec4(newAdaptation, newAdaptation, newAdaptation, 1.0);
		#endif
		}
	}
}
