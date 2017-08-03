in vec2 v2f_texCoord;
in vec4 v2f_clipCoord;
in vec4 v2f_motionVec;
in vec4 v2f_motionVecPrev;

out vec4 o_fragColor : FRAG_COLOR;

uniform float shutterSpeed;
//uniform float motionBlurID;
uniform sampler2D albedoMap;
uniform float perforatedAlpha;
uniform sampler2D depthMap;

void main() {
	float screenDepth = tex2Dproj(depthMap, v2f_clipCoord).x;
	/*if (gl_FragCoord.z > screenDepth + 0.0001) {
		discard;
	}*/

	vec3 motionVec = v2f_motionVec.xyz / v2f_motionVec.w;
	vec3 motionVecPrev = v2f_motionVecPrev.xyz / v2f_motionVecPrev.w;

	vec2 velocity = (motionVec.xy - motionVecPrev.xy) * shutterSpeed;
	velocity = dot(velocity.xy, velocity.xy) > 0.000001 ? velocity : vec2(0.0, 0.0);

#ifdef PERFORATED
	float alpha = tex2D(albedoMap, v2f_texCoord).a;
	if (alpha < perforatedAlpha) {
		discard;
	}
#endif

	o_fragColor = vec4(velocity, motionVec.z/*motionBlurID*/, 1.0);
}
