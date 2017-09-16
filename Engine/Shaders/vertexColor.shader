shader "vertexColor" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in LOWP vec4 in_color : COLOR;

		out LOWP vec4 v2f_color;

		uniform mat4 modelViewProjectionMatrix;

		void main() {
			gl_Position = modelViewProjectionMatrix * in_position;
			v2f_color = in_color;
		}
	}

	glsl_fp {
		in LOWP vec4 v2f_color;

		out vec4 o_fragColor : FRAG_COLOR;

		void main() {
			o_fragColor = v2f_color;
		}
	}
}
