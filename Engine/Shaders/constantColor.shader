shader "constantColor" {
	generatePerforatedVersion
	generateGpuSkinningVersion

	glsl_vp {
		in vec4 in_position : POSITION;

		uniform mat4 modelViewProjectionMatrix;

		void main() {
			vec4 localVertex;

		#ifdef GPU_SKINNING
			skinningMatrix(in_position, localVertex);
		#else
			localVertex = in_position;
		#endif

			gl_Position = modelViewProjectionMatrix * localVertex;
		}
	}

	glsl_fp {
		out vec4 o_fragColor : FRAG_COLOR;

		uniform vec4 color;

		void main() {
			o_fragColor = color;
		}
	}
}
