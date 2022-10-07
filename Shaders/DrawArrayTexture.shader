shader "drawArrayTexture" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec3 in_str : TEXCOORD0;

		out vec3 v2f_texCoord;

		void main() {
			v2f_texCoord = in_str;

			gl_Position = in_position;
		}
	}

	glsl_fp {
		in vec3 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2DArray tex0;

		void main() {
			o_fragColor = tex2Darray(tex0, v2f_texCoord.xyz);
			//o_fragColor = vec4(1.0, 1.0, 1.0, 1.0);
		}
	}
}
