shader "colorTransform" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D tex;
		uniform vec4 colorMatrixS;
		uniform vec4 colorMatrixT;
		uniform vec4 colorMatrixR;

		void main() {
			vec4 screenColor = vec4(tex2D(tex, v2f_texCoord.st).xyz, 1.0);

			float r = dot(screenColor.xyzw, colorMatrixS.xyzw);
			float g = dot(screenColor.xyzw, colorMatrixT.xyzw);
			float b = dot(screenColor.xyzw, colorMatrixR.xyzw);

			o_fragColor = vec4(r, g, b, screenColor.w);
		}
	}
}
