shader "kawaseBlur" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec2 in_st : TEXCOORD0;

		out vec4 v2f_texCoord0;
		out vec4 v2f_texCoord1;
		out vec2 v2f_texCoord2;
		out vec2 v2f_texCoord4;
		
		uniform vec2 sampleOffsets[4];

		void main() {
			gl_Position = in_position;
			v2f_texCoord0.xy = in_st.xy;
			v2f_texCoord0.zw = in_st.xy + sampleOffsets[0];
			v2f_texCoord1.xy = in_st.xy + sampleOffsets[1];
			v2f_texCoord1.zw = in_st.xy + sampleOffsets[2];
			v2f_texCoord2.xy = in_st.xy + sampleOffsets[3];
		}
	}

	glsl_fp {
		in vec4 v2f_texCoord0;
		in vec4 v2f_texCoord1;
		in vec2 v2f_texCoord2;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D tex0;

		void main() {
			vec4 sum = tex2D(tex0, v2f_texCoord0.xy);
			sum += tex2D(tex0, v2f_texCoord0.zw);
			sum += tex2D(tex0, v2f_texCoord1.xy);
			sum += tex2D(tex0, v2f_texCoord1.zw);
			sum += tex2D(tex0, v2f_texCoord2.xy);

			o_fragColor = sum / 5.0;
		}
	}
}
