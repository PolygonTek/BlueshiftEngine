shader "postCameraMotionBlur" {
	glsl_vp {
		in vec4 in_position : POSITION;

		out vec4 v2f_motionVec;
		out vec4 v2f_motionVecPrev;

		uniform vec3 worldViewPos;
		uniform mat4 currViewProjectionMatrix;	// currProjM * currViewM
		uniform mat4 prevViewProjectionMatrix;	// currProjM * prevViewM

		void main() {
			vec4 pos = vec4(normalize(in_position.xyz) * 25.0 + worldViewPos, 1.0);

			v2f_motionVec = currViewProjectionMatrix * pos;
			v2f_motionVecPrev = prevViewProjectionMatrix * pos;

			gl_Position = v2f_motionVec;

			v2f_motionVec.xy = (v2f_motionVec.xy + v2f_motionVec.ww) * 0.5;
			v2f_motionVecPrev.xy = (v2f_motionVecPrev.xy + v2f_motionVecPrev.ww) * 0.5;
		}
	}

	glsl_fp {
		in vec4 v2f_motionVec;
		in vec4 v2f_motionVecPrev;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D colorMap;
		uniform sampler2D depthMap;
		uniform float shutterSpeed;
		uniform float blurScale;
		uniform float maxBlur;

		const float numSamples = 8.0;
		const float sampleStep = 2.0 / numSamples;

		void main() {
			vec2 currVec = v2f_motionVec.xy / v2f_motionVec.w;
			vec2 prevVec = v2f_motionVecPrev.xy / v2f_motionVecPrev.w;

			vec2 velocityVec = (currVec - prevVec) * shutterSpeed * blurScale * 0.5;
			float len = length(velocityVec);

			velocityVec.xy /= len;
			velocityVec.xy *= min(len, maxBlur);

			vec4 midColor = tex2D(colorMap, currVec);
			float depth = tex2D(depthMap, currVec).x;

			vec4 accumColor = vec4(0.0, 0.0, 0.0, 0.0);

			for (float s = -1.0; s < 1.0; s += sampleStep) {
				vec2 tc = currVec - velocityVec * s;
				//float depthMask = tex2D(depthMap, tc).x;
				//tc += velocityVec * s * (1.0 - depthMask);
				accumColor += tex2D(colorMap, tc);
			}

			accumColor /= numSamples;

			// Remove scene bleeding from 1st player hands
			o_fragColor = mix(midColor, accumColor, (depth - 0.1) * (1.0 / 0.9));
			//o_fragColor = vec4(velocityVec, 0.0, 1.0);
		}
	}
}
