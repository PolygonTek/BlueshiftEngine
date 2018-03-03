shader "passThruColor" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D tex0;
		uniform vec4 color;

		void main() {
			o_fragColor = tex2D(tex0, v2f_texCoord.st) * color;
		}
	}
}
