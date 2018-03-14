shader "shadowVolume" {
	glsl_vp {
		in vec4 in_position : POSITION;

		uniform mat4 modelViewProjectionMatrix;
		uniform vec3 localLightOrigin;

		void main() {
			vec3 v = in_position.xyz - localLightOrigin.xyz;
			v += in_position.w * localLightOrigin.xyz;

			gl_Position = modelViewProjectionMatrix * vec4(v, in_position.w);
		}
	}

	glsl_fp {
		out vec4 o_fragColor : FRAG_COLOR;

		uniform vec4 showColor;

		void main() {
			o_fragColor = showColor;
		}
	}
}
