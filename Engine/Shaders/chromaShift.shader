shader "chromaShift" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D tex0;
		uniform vec3 shift;

		void main() {
  		vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

			color.x = tex2D(tex0, (v2f_texCoord.xy - 0.5) * (1.0 - shift.x) + 0.5).x;
			color.y = tex2D(tex0, (v2f_texCoord.xy - 0.5) * (1.0 - shift.y) + 0.5).y;
			color.z = tex2D(tex0, (v2f_texCoord.xy - 0.5) * (1.0 - shift.z) + 0.5).z;

			o_fragColor = color;
		}
	}
}
