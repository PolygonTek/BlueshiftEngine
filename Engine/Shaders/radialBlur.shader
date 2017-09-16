shader "radialBlur" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D tex0;
		uniform vec2 screenPos;
		uniform float invRadius;
		uniform float blurAmount;//?

		void main() {
			vec2 blurVec = screenPos.xy - v2f_texCoord.xy;

			float blurDist = 1.0 - dot(blurVec.xy * invRadius, blurVec.xy * invRadius);
			saturate(blurDist);

			float b = blurAmount * blurDist * blurDist;

			const int samples = 8;
			const float weight = 1.0 / float(samples);

			vec4 accum = vec4(0.0, 0.0, 0.0, 0.0);

			for (int i = 0; i < samples; i++) {
				vec4 curr = tex2D(tex0, v2f_texCoord.xy + blurVec.xy * float(i) * b);
				accum += curr;
			}

			o_fragColor = accum * weight;
		}
	}
}
