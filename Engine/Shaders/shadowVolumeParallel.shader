shader "shadowVolumeParallel" {
	glsl_vp {
		in vec4 in_position : POSITION;

		uniform mat4 modelViewProjectionMatrix;
		uniform vec3 localLightDir;

		void main() {
			vec3 v = (1.0 - in_position.w) * -localLightDir.xyz;
			v += in_position.w * in_position.xyz;

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
