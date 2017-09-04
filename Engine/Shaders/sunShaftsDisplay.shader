shader "sunShaftsDisplay" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D screenMap;
		uniform sampler2D sunShaftsMap;
		uniform LOWP vec4 sunColor;

		void main() {
			vec4 screen = tex2D(screenMap, v2f_texCoord.st);
			vec4 sunShafts = tex2D(sunShaftsMap, v2f_texCoord.st);

			float shaftsMask = saturate(1.00001 - sunShafts.w) * 1.0;

			vec4 outColor = screen + sunShafts.xyzz * 1.0 * sunColor * max(1.0 - screen, 0.0);
			o_fragColor = outColor;
		}
	}
}
