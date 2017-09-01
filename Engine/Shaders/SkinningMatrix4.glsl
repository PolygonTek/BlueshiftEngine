in vec4 in_weightIndex : WEIGHT_INDEX;
in vec4 in_weightValue : WEIGHT_VALUE;

#ifdef VTF_SKINNING
	#ifdef USE_BUFFER_TEXTURE
		uniform samplerBuffer jointsMap;
		#define TC_TYPE int
	#else
		uniform sampler2D jointsMap;
		#define TC_TYPE vec2
	#endif

	#ifdef INSTANCED_DRAW
		uniform TC_TYPE tcBase[256];	// indexed by gl_InstanceIDARB
		#define BASE_TC	tcBase[gl_InstanceIDARB]
	#else
		uniform TC_TYPE tcBase;
		#define BASE_TC	tcBase
	#endif

	uniform vec2 invJointsMapSize;
	uniform int jointIndexOffsetCurr;
	uniform int jointIndexOffsetPrev;
#else
    uniform vec4 joints[MAX_SHADER_JOINTSX3];   // 4x3 matrix
#endif

void accumulateJointMatrices(out vec4 R0, out vec4 R1, out vec4 R2, int jointIndexOffset) {
	float invNorm = 1.0 / dot(in_weightValue, vec4(1.0));
	vec4 w = in_weightValue * invNorm;

#ifdef VTF_SKINNING
	#ifdef USE_BUFFER_TEXTURE
		int baseS = BASE_TC + (jointIndexOffset + int(in_weightIndex.x)) * 3;
		R0 = w.x * texelFetch(jointsMap, baseS + 0);
		R1 = w.x * texelFetch(jointsMap, baseS + 1);
		R2 = w.x * texelFetch(jointsMap, baseS + 2);

		if (w.y > 0.0) {
			baseS = BASE_TC + (jointIndexOffset + int(in_weightIndex.y)) * 3;
			R0 += w.y * texelFetch(jointsMap, baseS + 0);
			R1 += w.y * texelFetch(jointsMap, baseS + 1);
			R2 += w.y * texelFetch(jointsMap, baseS + 2);

			if (w.z > 0.0) {
				baseS = BASE_TC + (jointIndexOffset + int(in_weightIndex.z)) * 3;
				R0 += w.z * texelFetch(jointsMap, baseS + 0);
				R1 += w.z * texelFetch(jointsMap, baseS + 1);
				R2 += w.z * texelFetch(jointsMap, baseS + 2);

				if (w.w > 0.0) {
					baseS = BASE_TC + (jointIndexOffset + int(in_weightIndex.w)) * 3;
					R0 += w.w * texelFetch(jointsMap, baseS + 0);
					R1 += w.w * texelFetch(jointsMap, baseS + 1);
					R2 += w.w * texelFetch(jointsMap, baseS + 2);
				}
			}
		}
	#else
		vec2 baseST = BASE_TC + vec2((float(jointIndexOffset) + in_weightIndex.x) * 3.0, 0.0);
		R0 = w.x * tex2Dlod(jointsMap, vec4((baseST + vec2(0.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
		R1 = w.x * tex2Dlod(jointsMap, vec4((baseST + vec2(1.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
		R2 = w.x * tex2Dlod(jointsMap, vec4((baseST + vec2(2.0, 0.0)) * invJointsMapSize, 0.0, 0.0));

		if (w.y > 0.0) {
			baseST = BASE_TC + vec2((float(jointIndexOffset) + in_weightIndex.y) * 3.0, 0.0);
			R0 += w.y * tex2Dlod(jointsMap, vec4((baseST + vec2(0.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
			R1 += w.y * tex2Dlod(jointsMap, vec4((baseST + vec2(1.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
			R2 += w.y * tex2Dlod(jointsMap, vec4((baseST + vec2(2.0, 0.0)) * invJointsMapSize, 0.0, 0.0));

			if (w.z > 0.0) {
				baseST = BASE_TC + vec2((float(jointIndexOffset) + in_weightIndex.z) * 3.0, 0.0);
				R0 += w.z * tex2Dlod(jointsMap, vec4((baseST + vec2(0.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
				R1 += w.z * tex2Dlod(jointsMap, vec4((baseST + vec2(1.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
				R2 += w.z * tex2Dlod(jointsMap, vec4((baseST + vec2(2.0, 0.0)) * invJointsMapSize, 0.0, 0.0));

				if (w.w > 0.0) {
					baseST = BASE_TC + vec2((float(jointIndexOffset) + in_weightIndex.w) * 3.0, 0.0);
					R0 += w.w * tex2Dlod(jointsMap, vec4((baseST + vec2(0.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
					R1 += w.w * tex2Dlod(jointsMap, vec4((baseST + vec2(1.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
					R2 += w.w * tex2Dlod(jointsMap, vec4((baseST + vec2(2.0, 0.0)) * invJointsMapSize, 0.0, 0.0));
				}
			}
		}
	#endif
#else
	int weightIndex = (jointIndexOffset + int(in_weightIndex.x)) * 3;
	R0 = w.x * joints[weightIndex + 0];
	R1 = w.x * joints[weightIndex + 1];
	R2 = w.x * joints[weightIndex + 2];

	weightIndex = (jointIndexOffset + int(in_weightIndex.y)) * 3;
	R0 += w.y * joints[weightIndex + 0];
	R1 += w.y * joints[weightIndex + 1];
	R2 += w.y * joints[weightIndex + 2];

	weightIndex = (jointIndexOffset + int(in_weightIndex.z)) * 3;
	R0 += w.z * joints[weightIndex + 0];
	R1 += w.z * joints[weightIndex + 1];
	R2 += w.z * joints[weightIndex + 2];

	weightIndex = (jointIndexOffset + int(in_weightIndex.w)) * 3;
	R0 += w.w * joints[weightIndex + 0];
	R1 += w.w * joints[weightIndex + 1];
	R2 += w.w * joints[weightIndex + 2];
#endif
}

$include "SkinningMatrixCommon.glsl"
