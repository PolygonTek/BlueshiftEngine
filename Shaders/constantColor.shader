shader "constantColor" {
	generatePerforatedVersion
	generateGpuSkinningVersion
    generateGpuInstancingVersion

	glsl_vp {
		in vec4 in_position : POSITION;

        #ifdef INSTANCING
            $include "Instancing.vp"
        #else
            uniform vec4 localToWorldMatrixS;
            uniform vec4 localToWorldMatrixT;
            uniform vec4 localToWorldMatrixR;
        #endif

        uniform mat4 viewProjectionMatrix;
		uniform mat4 modelViewProjectionMatrix;

		void main() {
			vec4 localPos;

		#ifdef GPU_SKINNING
			skinningMatrix(in_position, localPos);
		#else
			localPos = in_position;
		#endif

        #ifdef INSTANCING
            vec4 worldPos;
            worldPos.x = dot(localToWorldMatrixS, localPos);
            worldPos.y = dot(localToWorldMatrixT, localPos);
            worldPos.z = dot(localToWorldMatrixR, localPos);
            worldPos.w = 1.0;
            gl_Position = viewProjectionMatrix * worldPos;
        #else
            gl_Position = modelViewProjectionMatrix * localPos;
        #endif
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
