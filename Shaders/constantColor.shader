shader "constantColor" {
	generatePerforatedVersion
	generateGpuSkinningVersion
    generateGpuInstancingVersion

	glsl_vp {
		in vec4 in_position : POSITION;

		uniform mat4 modelViewProjectionMatrix;

		void main() {
			vec4 localPos;

		#ifdef GPU_SKINNING
			skinningMatrix(in_position, localPos);
		#else
			localPos = in_position;
		#endif

			gl_Position = modelViewProjectionMatrix * localPos;
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
