shader "blur5x" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		#define NUM_SAMPLES 5
		uniform sampler2D tex0;
		uniform float weights[NUM_SAMPLES];
		uniform vec2 texelOffset;

		void main() {
			vec4 c = vec4(0.0, 0.0, 0.0, 0.0);

			float offset = (float(NUM_SAMPLES) - 1.0) / 2.0;

			for (int i = 0; i < NUM_SAMPLES; i++) {
				c += weights[i] * tex2D(tex0, v2f_texCoord.st - (float(i) - offset) * texelOffset);
			}

			o_fragColor = c;
		}
	}
}
