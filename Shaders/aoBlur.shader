shader "aoBlur" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec2 in_st : TEXCOORD0;

		out vec2 v2f_texCoord0;
		out vec4 v2f_texCoord1;
		out vec4 v2f_texCoord2;

		uniform vec2 texelOffset;

		void main() {
			gl_Position = in_position;
			v2f_texCoord0.xy = in_st.xy;
			v2f_texCoord1.xy = in_st.xy + -3.0 * texelOffset;
			v2f_texCoord1.zw = in_st.xy + -1.5 * texelOffset;
			v2f_texCoord2.xy = in_st.xy + +1.5 * texelOffset;
			v2f_texCoord2.zw = in_st.xy + +3.0 * texelOffset;
		}
	}

	glsl_fp {
		in vec2 v2f_texCoord0;
		in vec4 v2f_texCoord1;
		in vec4 v2f_texCoord2;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D colorMap;
		uniform sampler2D depthMap;
		uniform vec2 projComp;
		//uniform sampler2D normalMap;

		float depthToViewZ(float depth) {
			float ndc = depth * 2.0 - 1.0;
			return projComp.y / (ndc + projComp.x);
		}

		float weightSample(float depth, float baseViewZ) {
			float viewz = depthToViewZ(depth);
			float tolerance = baseViewZ / 48.0;
			return abs(viewz - baseViewZ) < tolerance ? 1.0 : 0.0;
		}

		void main() {
			float depth = tex2D(depthMap, v2f_texCoord0.xy).r;
			float baseViewZ = depthToViewZ(depth);
			float w0 = weightSample(tex2D(depthMap, v2f_texCoord1.xy).r, baseViewZ);
			float w1 = weightSample(tex2D(depthMap, v2f_texCoord1.zw).r, baseViewZ);
			float w3 = weightSample(tex2D(depthMap, v2f_texCoord2.xy).r, baseViewZ);
			float w4 = weightSample(tex2D(depthMap, v2f_texCoord2.zw).r, baseViewZ);
			float total_w = 1.0 + w0 + w1 + w3 + w4;

			vec4 c = tex2D(colorMap, v2f_texCoord0.xy);
			c += tex2D(colorMap, v2f_texCoord1.xy) * w0;
			c += tex2D(colorMap, v2f_texCoord1.zw) * w1;
			c += tex2D(colorMap, v2f_texCoord2.xy) * w3;
			c += tex2D(colorMap, v2f_texCoord2.zw) * w4;
			o_fragColor = c / total_w;
		}
	}
}
