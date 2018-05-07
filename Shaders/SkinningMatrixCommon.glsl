void skinningMatrix(vec4 position, out vec4 transformedPosition) {
	vec4 R0, R1, R2;
	
#ifdef OBJECT_MOTION_BLUR
	accumulateJointMatrices(R0, R1, R2, jointIndexOffset[0]);
#else
	accumulateJointMatrices(R0, R1, R2, 0);
#endif
	
	// transform vertex
	transformedPosition.x = dot(R0, position);
	transformedPosition.y = dot(R1, position);
	transformedPosition.z = dot(R2, position);
	transformedPosition.w = 1.0;
}

void skinningMatrix(vec4 position, out vec4 transformedPosition, vec3 normal, out vec3 transformedNormal) {
 	vec4 R0, R1, R2;

#ifdef OBJECT_MOTION_BLUR
	accumulateJointMatrices(R0, R1, R2, jointIndexOffset[0]);
#else
	accumulateJointMatrices(R0, R1, R2, 0);
#endif
	
	// transform vertex
	transformedPosition.x = dot(R0, position);
	transformedPosition.y = dot(R1, position);
	transformedPosition.z = dot(R2, position);
	transformedPosition.w = 1.0;
	
	// transform normal
	transformedNormal.x = dot(R0.xyz, normal);
	transformedNormal.y = dot(R1.xyz, normal);
	transformedNormal.z = dot(R2.xyz, normal);
	
}

void skinningMatrix(vec4 position, out vec4 transformedPosition, vec3 normal, out vec3 transformedNormal, vec4 tangent, out vec3 transformedTangent, out vec3 transformedBitangent) {
	vec4 R0, R1, R2;

#ifdef OBJECT_MOTION_BLUR
	accumulateJointMatrices(R0, R1, R2, jointIndexOffset[0]);
#else
	accumulateJointMatrices(R0, R1, R2, 0);
#endif
	
	// transform vertex
	transformedPosition.x = dot(R0, position);
	transformedPosition.y = dot(R1, position);
	transformedPosition.z = dot(R2, position);
	transformedPosition.w = 1.0;

	// transform normal
	transformedNormal.x = dot(R0.xyz, normal);
	transformedNormal.y = dot(R1.xyz, normal);
	transformedNormal.z = dot(R2.xyz, normal);
	
	// transform tangent
	transformedTangent.x = dot(R0.xyz, tangent.xyz);
	transformedTangent.y = dot(R1.xyz, tangent.xyz);
	transformedTangent.z = dot(R2.xyz, tangent.xyz);

	// derive bitangent
	transformedBitangent = cross(transformedNormal, transformedTangent);
	transformedBitangent = transformedBitangent * tangent.w;
}

void skinningPrevMatrix(vec4 position, out vec4 transformedPosition) {
	vec4 R0, R1, R2;

#ifdef OBJECT_MOTION_BLUR
	accumulateJointMatrices(R0, R1, R2, jointIndexOffset[1]);
#else
	accumulateJointMatrices(R0, R1, R2, 0);
#endif

	// transform vertex
	transformedPosition.x = dot(R0, position);
	transformedPosition.y = dot(R1, position);
	transformedPosition.z = dot(R2, position);
	transformedPosition.w = 1.0;
}