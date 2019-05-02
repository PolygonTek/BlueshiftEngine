shader "WriteValue" {
	glsl_vp {
		in vec4 in_position : POSITION;

		void main() {
			gl_Position = in_position;
		}
	}

	glsl_fp {
		out vec4 o_fragColor : FRAG_COLOR;

		uniform vec4 value;

		void main() {
			o_fragColor = value;
		}
	}
}
