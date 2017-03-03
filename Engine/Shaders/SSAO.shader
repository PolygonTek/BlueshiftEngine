shader "SSAO" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec2 in_st : TEXCOORD0;

		out vec4 v2f_texCoord0;
		out vec2 v2f_texCoord1;

		uniform vec2 screenSize;
		uniform vec2 screenTanHalfFov;		// leftTop / zNear

		void main() {
			gl_Position = in_position;
			v2f_texCoord0.xy = in_st.xy;
			v2f_texCoord0.zw = in_st.xy * screenSize.xy / 4.0; // divided by random dir texture size
			v2f_texCoord1.xy = mix(-screenTanHalfFov, screenTanHalfFov, in_st);
		}
	}

	glsl_fp {
		in vec4 v2f_texCoord0;
		in vec2 v2f_texCoord1;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D depthMap;
		uniform sampler2D downscaledDepthMap;
		uniform sampler2D normalMap;
		uniform sampler2D randomDir4x4Map;

		uniform vec3 randomKernel[8];
		uniform vec4 projComp1;
		uniform vec4 projComp2;
		uniform float radius;
		uniform float validRange;
		uniform float threshold;
		uniform float power;
		uniform float intensity;

		float depthToViewZ(float depth) {
			float ndc = depth * 2.0 - 1.0;
			return -projComp2.y / (ndc + projComp2.x);
		}

		vec4 depthToViewZ(vec4 depth) {
			vec4 ndc = depth * 2.0 - 1.0;
			return -projComp2.yyyy / (ndc + projComp2.xxxx);
		}

		vec2 getScreenXY(vec3 viewPos) {
			vec2 screenPos = projComp1.xy * viewPos.xy / viewPos.z + projComp1.zw;
			return screenPos * 0.5 + 0.5;
		}

		void main() {
			float depth = tex2D(depthMap, v2f_texCoord0.xy).x;
			if (depth >= 1.0) {
				o_fragColor = vec4(1.0);
				return;
			}

			vec3 Rn = normalize(tex2D(randomDir4x4Map, v2f_texCoord0.zw).xyz * 2.0 - 1.0);

			vec3 viewPos;
			viewPos.z = depthToViewZ(depth);
			viewPos.xy = viewPos.z * v2f_texCoord1.xy;

			float distanceScale = min((4.0 - viewPos.z) / 100.0, 1.0);

			float vRayLen = radius * distanceScale;
			vec2 vRange = vec2(validRange, validRange * 0.5) * distanceScale;
			vec2 vThreshold = vec2(threshold, threshold * 0.5) * distanceScale;

			vec4 sampleDepth[4];
			vec4 rayz[4];
			vec3 ray;

			vec4 vAo = vec4(0.0);
			vec4 vPower = vec4(power);
			vec4 vDef = vec4(0.45);

			for (int s = 0; s < 4; s++) {
				ray = reflect(randomKernel[s], Rn) * vRayLen;
				sampleDepth[0][s] = tex2D(downscaledDepthMap, getScreenXY(viewPos + ray)).x;
				rayz[0][s] = ray.z;

		#ifdef HIGH_QUALITY_SSAO
				ray *= 0.5;
				sampleDepth[2][s] = tex2D(downscaledDepthMap, getScreenXY(viewPos + ray)).x;
				rayz[2][s] = ray.z;
		#endif

				ray = reflect(randomKernel[4 + s], Rn) * vRayLen;
				sampleDepth[1][s] = tex2D(downscaledDepthMap, getScreenXY(viewPos + ray)).x;
				rayz[1][s] = ray.z;

		#ifdef HIGH_QUALITY_SSAO
				ray *= 0.5;
				sampleDepth[3][s] = tex2D(downscaledDepthMap, getScreenXY(viewPos + ray)).x;
				rayz[3][s] = ray.z;
		#endif
			}

			vec4 ray_z_occlusion[4];
			vec4 contrib[4];
			vec4 invalid[4];

			ray_z_occlusion[0] = (depthToViewZ(sampleDepth[0]) - rayz[0]) - viewPos.z;
			ray_z_occlusion[1] = (depthToViewZ(sampleDepth[1]) - rayz[1]) - viewPos.z;

			contrib[0] = pow(clamp((ray_z_occlusion[0] - vThreshold.x) / vRange.x, 0.0, 1.0), vPower);
			contrib[1] = pow(clamp((ray_z_occlusion[1] - vThreshold.x) / vRange.x, 0.0, 1.0), vPower);

			invalid[0] = clamp((ray_z_occlusion[0] - vRange.x) / 16.0, 0.0, 1.0);
			invalid[1] = clamp((ray_z_occlusion[1] - vRange.x) / 16.0, 0.0, 1.0);

			vAo += mix(contrib[0], vDef, invalid[0]);
			vAo += mix(contrib[1], vDef, invalid[1]);

		#ifdef HIGH_QUALITY_SSAO
			ray_z_occlusion[2] = (depthToViewZ(sampleDepth[2]) - rayz[2]) - viewPos.z;
			ray_z_occlusion[3] = (depthToViewZ(sampleDepth[3]) - rayz[3]) - viewPos.z;

			contrib[2] = pow(clamp((ray_z_occlusion[2] - vThreshold.y) / vRange.y, 0.0, 1.0), vPower);
			contrib[3] = pow(clamp((ray_z_occlusion[3] - vThreshold.y) / vRange.y, 0.0, 1.0), vPower);

			invalid[2] = clamp((ray_z_occlusion[2] - vRange.y) / 8.0, 0.0, 1.0);
			invalid[3] = clamp((ray_z_occlusion[3] - vRange.y) / 8.0, 0.0, 1.0);

			vAo += mix(contrib[2], vDef, invalid[2]);
			vAo += mix(contrib[3], vDef, invalid[3]);
		#endif

		#if defined(HIGH_QUALITY_SSAO)
			vAo /= 16.0;
		#else
			vAo /= 8.0;
		#endif

			float Ao = 1.0 - min(dot(vAo, vec4(1.0, 1.0, 1.0, 1.0)), 1.0);
			Ao = smoothstep(0.35, 0.85, Ao);

			o_fragColor = vec4(pow(Ao, intensity));
		}
	}
}
