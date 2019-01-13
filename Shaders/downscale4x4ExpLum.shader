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
			float logLum = 0.0;

			for (int i = 0; i < SAMPLES; i++) {
				logLum += tex2D(tex0, v2f_texCoord.st + sampleOffsets[i]).x;
			}

            // Geometric mean of screen luminances = e^{ sum^N { log(pixel(x, y)) } / N }
			float avgLuminance = exp(logLum / float(SAMPLES));

			o_fragColor = vec4(avgLuminance, avgLuminance, avgLuminance, 1.0);
		}
	}
}
