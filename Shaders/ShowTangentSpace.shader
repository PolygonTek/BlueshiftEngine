shader "showTangentSpace" {
	glsl_vp {
		in vec4 in_position : POSITION;
	#ifdef PACKED_VERTEX_TANGENT_FRAME
		in HIGHP uint in_tangentFrame : NORMAL;
	#else
		in LOWP vec3 in_normal : NORMAL;
		in LOWP vec4 in_tangent : TEXCOORD1;
	#endif

		out LOWP vec4 v2f_color;

		uniform int tangentIndex;
		uniform HIGHP mat4 modelViewProjectionMatrix;

		void main() {
			vec3 tangents[3];

		#ifdef PACKED_VERTEX_TANGENT_FRAME
			vec3 vNormal;
			vec4 vTangent;
			unpackTangentFrame(in_tangentFrame, vNormal, vTangent);
		#else
			vec3 vNormal = in_normal.xyz;
			vec4 vTangent = in_tangent;
		#endif

			tangents[0] = vTangent.xyz;
			tangents[1] = cross(vNormal, vTangent.xyz) * vTangent.w;
			tangents[2] = vNormal;

			tangents[0] = (tangents[0] + vec3(1.0, 1.0, 1.0)) * 0.5;
			tangents[1] = (tangents[1] + vec3(1.0, 1.0, 1.0)) * 0.5;
			tangents[2] = (tangents[2] + vec3(1.0, 1.0, 1.0)) * 0.5;

			v2f_color.xyz = tangents[tangentIndex];
			v2f_color.w = 1.0;

			gl_Position = modelViewProjectionMatrix * in_position;
		}
	}

	glsl_fp {
		in vec4 v2f_color;

		out vec4 o_fragColor : FRAG_COLOR;

		void main() {
			o_fragColor = v2f_color;
		}
	}
}
