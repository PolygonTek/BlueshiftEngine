shader "screenGrayScale" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D tex0;

		void main() {
			vec3 color = tex2D(tex0, v2f_texCoord.st).rgb;
			float greyscale = dot(vec3(0.299, 0.587, 0.114), color);
			color = vec3(greyscale, greyscale, greyscale);

			o_fragColor = vec4(color, 1.0);
		}
	}
}
