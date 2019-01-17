shader "downscale4x4LogLum" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
        $include "Colors.glsl"

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
			float sumLogLum = 0.0;

            for (int i = 0; i < SAMPLES; i++) {
                vec4 color = tex2D(tex0, v2f_texCoord.st + sampleOffsets[i]).rgba;

                sumLogLum += log(max(GetLuma(color.rgb), 0.0001));
            }

            float avgLogLum = sumLogLum / float(SAMPLES);

            o_fragColor = vec4(avgLogLum, avgLogLum, avgLogLum, 1.0);
		}
	}
}
