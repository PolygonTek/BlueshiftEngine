shader "downscale4x4LogLum" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		#ifdef LOGLUV_HDR
		$include "logluv.glsl"
		#endif

		#if 1// BILINEAR_FP16
		#define SAMPLES 4
		#else
		#define SAMPLES 16
		#endif

		uniform sampler2D tex0;
		uniform vec2 sampleOffsets[SAMPLES];

		const vec3 lumVector = vec3(0.2125, 0.7154, 0.0721);

		void main() {
			float lumSum = 0.0;
			float lum;

			for (int i = 0; i < SAMPLES; i++) {
		#ifdef LOGLUV_HDR
				lum = dot(decodeLogLuv(tex2D(tex0, v2f_texCoord.st + sampleOffsets[i])).xyz, lumVector);
		#else
				lum = dot(tex2D(tex0, v2f_texCoord.st + sampleOffsets[i]).xyz, lumVector);
		#endif
				lum = max(lum, 0.08);

				lumSum += log(lum + 0.0001);
			}
			lumSum /= float(SAMPLES);

		#ifdef LOGLUV_HDR
			o_fragColor = encodeLogLuv(vec3(lumSum, lumSum, lumSum));
		#else
			o_fragColor = vec4(lumSum, lumSum, lumSum, 1.0);
		#endif
		}
	}
}
