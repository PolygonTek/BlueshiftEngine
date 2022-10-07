shader "constantColor" {
	generatePerforatedVersion
	generateGpuSkinningVersion
    generateGpuInstancingVersion

	glsl_vp {
		in vec4 in_position : POSITION;

        #if defined(INSTANCING)
            $include "Instancing.vp"
        #else
            uniform HIGHP mat4x3 localToWorldMatrix;
        #endif

        #ifdef GPU_SKINNING
            #if defined(GPU_SKINNING_1_WEIGHTS)
                $include "SkinningMatrix1.glsl"
            #elif defined(GPU_SKINNING_4_WEIGHTS)
                $include "SkinningMatrix4.glsl"
            #elif defined(GPU_SKINNING_8_WEIGHTS)
                $include "SkinningMatrix8.glsl"
            #endif
        #endif

        uniform HIGHP mat4 viewProjectionMatrix;
		uniform HIGHP mat4 modelViewProjectionMatrix;

		void main() {
			vec4 localPos;

		#ifdef GPU_SKINNING
			skinningMatrix(in_position, localPos);
		#else
			localPos = in_position;
		#endif

        #ifdef INSTANCING
            gl_Position = viewProjectionMatrix * vec4(localToWorldMatrix * localPos, 1.0);
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
