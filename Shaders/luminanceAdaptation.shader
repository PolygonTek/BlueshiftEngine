shader "luminanceAdaptation" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
        $include "Colors.glsl"

		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D currLuminanceMap;
		uniform sampler2D prevLuminanceMap;
		uniform float frameTime;

        const float adaptationRate = 1.25;

		void main() {
		#ifdef LOGLUV_HDR
			float currentLum = decodeLogLuv(tex2D(currLuminanceMap, vec2(0.0, 0.0))).x;
			float lastLum = decodeLogLuv(tex2D(prevLuminanceMap, vec2(0.0, 0.0))).x;
		#else
			float currentLum = tex2D(currLuminanceMap, vec2(0.0, 0.0)).x;
			float lastLum = tex2D(prevLuminanceMap, vec2(0.0, 0.0)).x;
		#endif

            // Adapt the luminance using Pattanaik's technique
            float adaptedLum = lastLum + (currentLum - lastLum) * (1.0 - exp(-frameTime * adaptationRate));

		#ifdef LOGLUV_HDR
			o_fragColor = encodeLogLuv(vec3(adaptedLum, adaptedLum, adaptedLum));
		#else
			o_fragColor = vec4(adaptedLum, adaptedLum, adaptedLum, 1.0);
		#endif
		}
	}
}
