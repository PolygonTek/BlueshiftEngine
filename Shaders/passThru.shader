shader "passThru" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D tex0;

		void main() {
			//vec2 ndc = gl_FragCoord.xy / screenSize;
			//o_fragColor = tex2D(tex0, ndc.xy);
			o_fragColor = tex2D(tex0, v2f_texCoord.st);
		}
	}
}
