shader "downscale4x4ExpLum" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		#if 1// BILINEAR_FP16
		#define SAMPLES 4
		#else
		#define SAMPLES 16
		#endif

		uniform sampler2D tex0;
		uniform vec2 sampleOffsets[SAMPLES];

		void main() {
			float lum = 0.0;

			for (int i = 0; i < SAMPLES; i++) {
				lum += tex2D(tex0, v2f_texCoord.st + sampleOffsets[i]).x;
			}
			lum = exp(lum / float(SAMPLES));

			o_fragColor = vec4(lum, lum, lum, 1.0);
		}
	}
}
