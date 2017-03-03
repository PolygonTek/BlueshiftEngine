shader "sunShaftsMaskGen" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec4 in_st : TEXCOORD0;

		out vec4 v2f_texCoord0;
		out vec4 v2f_texCoord1;
		out vec2 v2f_texCoord2;

		uniform vec2 texelSize;

		void main() {
			gl_Position = in_position;

			v2f_texCoord0 = vec4(
				in_st.st,
				in_st.st + vec2(+0.5, -1.5) * texelSize);

			v2f_texCoord1 = vec4(
				in_st.st + vec2(-1.5, -0.5) * texelSize, 
				in_st.st + vec2(-0.5, +1.5) * texelSize);

			v2f_texCoord2 = in_st.st + vec2(+1.5, +0.5) * texelSize;
		}
	}

	glsl_fp {
		in vec4 v2f_texCoord0;
		in vec4 v2f_texCoord1;
		in vec2 v2f_texCoord2;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D colorMap;
		uniform sampler2D depthMap;

		void main() {
		#if 0
			vec3 color = tex2D(colorMap, v2f_texCoord0.st).xyz +
				tex2D(colorMap, v2f_texCoord0.rq).xyz +
				tex2D(colorMap, v2f_texCoord1.st).xyz +
				tex2D(colorMap, v2f_texCoord1.rq).xyz +
				tex2D(colorMap, v2f_texCoord2.st).xyz;
			color *= 0.2;
			float depth = tex2D(depthMap, v2f_texCoord0.st).x +
				tex2D(depthMap, v2f_texCoord0.rq).x +
				tex2D(depthMap, v2f_texCoord1.st).x +
				tex2D(depthMap, v2f_texCoord1.rq).x +
				tex2D(depthMap, v2f_texCoord2.st).x;
			depth *= 0.2;
		#else
			vec3 color = tex2D(colorMap, v2f_texCoord0.st).xyz;
			float depth = tex2D(depthMap, v2f_texCoord0.st).x;
		#endif

			float shaftMask = 1.0 - depth;

			o_fragColor = vec4(color * depth, shaftMask);
		}
	}
}
