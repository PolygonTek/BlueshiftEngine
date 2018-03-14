shader "downscale4x4" {
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
			vec4 color = vec4(0.0, 0.0, 0.0, 0.0);

			for (int i = 0; i < SAMPLES; i++) {
				color += tex2D(tex0, v2f_texCoord.st + sampleOffsets[i]);
			}
			color /= float(SAMPLES);

			o_fragColor = color;
		}
	}
}
