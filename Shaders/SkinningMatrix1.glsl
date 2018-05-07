in int in_weightIndex : WEIGHT_INDEX;

#ifdef VTF_SKINNING
	#ifdef USE_BUFFER_TEXTURE
		uniform samplerBuffer jointsMap;
	#else
		uniform sampler2D jointsMap;
        uniform vec2 invJointsMapSize;
	#endif

	#ifndef INSTANCING
		uniform VTF_SKINNING_TC_TYPE skinningBaseTc;
	#endif

	uniform ivec2 jointIndexOffset;
#else
    uniform vec4 joints[MAX_SHADER_JOINTSX3];   // 4x3 matrix
#endif

void accumulateJointMatrices(out vec4 R0, out vec4 R1, out vec4 R2, int jointIndexOffset) {
#ifdef VTF_SKINNING
	#ifdef USE_BUFFER_TEXTURE
		int baseS = skinningBaseTc + (jointIndexOffset + in_weightIndex) * 3;
		R0 = texelFetch(jointsMap, baseS + 0);
		R1 = texelFetch(jointsMap, baseS + 1);
		R2 = texelFetch(jointsMap, baseS + 2);
	#else
		vec2 baseST = skinningBaseTc + vec2(float(jointIndexOffset + in_weightIndex) * 3.0, 0.0);
		R0 = tex2Dlod(jointsMap, vec4((baseST + vec2(0.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
		R1 = tex2Dlod(jointsMap, vec4((baseST + vec2(1.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
		R2 = tex2Dlod(jointsMap, vec4((baseST + vec2(2.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
	#endif
#else
	int weightIndex = (jointIndexOffset + in_weightIndex) * 3;
	R0 = joints[weightIndex + 0];
	R1 = joints[weightIndex + 1];
	R2 = joints[weightIndex + 2];
#endif
}

$include "SkinningMatrixCommon.glsl"
