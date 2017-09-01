shader "postObjectMotionBlur" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		#define NUM_SAMPLES 5
		uniform sampler2D albedoMap;
		//uniform sampler2D depthMap;
		uniform sampler2D velocityMap;
		uniform float motionBlurScale;
		uniform float motionBlurMax;

		const vec2 poisson[7] = vec2[](
			vec2(0.527837, -0.085868),
			vec2(-0.040088, 0.536087),
			vec2(-0.670445, -0.179949),
			vec2(-0.419418, -0.616039),
			vec2(0.440453, -0.639399),
			vec2(-0.757088, 0.349334),
			vec2(0.574619, 0.685879)
		);

		void main() {
			vec4 origColor = tex2D(albedoMap, v2f_texCoord.st);
			//float origDepth = tex2D(depthMap, v2f_texCoord.st).x;

			float numSamples = float(NUM_SAMPLES);
			float s = 0.0;

			vec4 accumColor;

			//for (int n = 0; n < 8; n++)
			{
				//vec2 offset = poisson[n] * 0.0333;// * clamp((1.0 - origDepth) * (1.0 - origDepth), 0.0, 1.0);
				vec2 offset = vec2(0.0, 0.0);

				vec4 pixelVelocity = tex2D(velocityMap, v2f_texCoord.st + offset);

				float len = dot(pixelVelocity.xy, pixelVelocity.xy);
				if (len == 0.0) {
					discard;
				}

				len = sqrt(len);
				pixelVelocity.xy /= len;
				pixelVelocity.xy *= min(len, motionBlurMax);

				accumColor.xyz = tex2D(albedoMap, v2f_texCoord.st).xyz;
				s = 1.0;

				for (float i = 1.0; i <= numSamples; i += 1.0) {
					vec2 lookup = v2f_texCoord.st - pixelVelocity.xy * (i / numSamples - 0.5) * motionBlurScale;

//						vec4 currVelocity = tex2D(velocityMap, lookup);

//						float currDepth = tex2D(depthMap, lookup).x;
//						if (currDepth < (currVelocity.z * 0.5 + 0.5))
//							continue;

					vec4 currColor = tex2D(albedoMap, lookup);

					accumColor.xyz += currColor.xyz;

					//float masking = pixelVelocity.z == currVelocity.z ? 1.0 : 0.0;
					//accumColor.w += masking * length(currVelocity);

					//float currDepth = tex2D(depthMap, lookup).x;
					//if (currDepth < origDepth)
					//	continue;

					s += 1.0;
				}
			}

			vec3 color = origColor.xyz;

			if (s > 0.0) {
				//float lp = accumColor.w / s;
				float lp = 1.0;
				color = vec3(mix(origColor.xyz, accumColor.xyz / s, clamp(lp * 3.0, 0.0, 1.0)));
			}

			o_fragColor = vec4(color, 1.0);
		}
	}
}
