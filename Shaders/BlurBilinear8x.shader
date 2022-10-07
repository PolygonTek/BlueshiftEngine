shader "blurBilinear8x" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec2 in_st : TEXCOORD0;

		out vec4 v2f_texCoord0;
		out vec4 v2f_texCoord1;
		out vec4 v2f_texCoord2;
		out vec4 v2f_texCoord3;

		uniform vec2 sampleOffsets[8];
		uniform vec2 texelOffset;

		void main() {
			gl_Position = in_position;
			v2f_texCoord0.xy = in_st.xy + sampleOffsets[0] * texelOffset;
			v2f_texCoord0.zw = in_st.xy + sampleOffsets[1] * texelOffset;
			v2f_texCoord1.xy = in_st.xy + sampleOffsets[2] * texelOffset;
			v2f_texCoord1.zw = in_st.xy + sampleOffsets[3] * texelOffset;
			v2f_texCoord2.xy = in_st.xy + sampleOffsets[4] * texelOffset;
			v2f_texCoord2.zw = in_st.xy + sampleOffsets[5] * texelOffset;
			v2f_texCoord3.xy = in_st.xy + sampleOffsets[6] * texelOffset;
			v2f_texCoord3.zw = in_st.xy + sampleOffsets[7] * texelOffset;
		}
	}

	glsl_fp {
		in vec4 v2f_texCoord0;
		in vec4 v2f_texCoord1;
		in vec4 v2f_texCoord2;
		in vec4 v2f_texCoord3;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D tex0;
		uniform float weights[8];

		void main() {
			vec4 col;
			vec4 sum = vec4(0.0, 0.0, 0.0, 0.0);

			col = tex2D(tex0, v2f_texCoord0.xy);
			sum += weights[0] * col;

			col = tex2D(tex0, v2f_texCoord0.zw);
			sum += weights[1] * col;

			col = tex2D(tex0, v2f_texCoord1.xy);
			sum += weights[2] * col;

			col = tex2D(tex0, v2f_texCoord1.zw);
			sum += weights[3] * col;

			col = tex2D(tex0, v2f_texCoord2.xy);
			sum += weights[4] * col;

			col = tex2D(tex0, v2f_texCoord2.zw);
			sum += weights[5] * col;

			col = tex2D(tex0, v2f_texCoord3.xy);
			sum += weights[6] * col;

			col = tex2D(tex0, v2f_texCoord3.zw);
			sum += weights[7] * col;

			o_fragColor = sum;
		}
	}
}
