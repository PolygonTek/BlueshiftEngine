shader "applyDOF" {
	/*glsl_vp {
			$include "clipQuad.vp"
		}

		glsl_fp {
			in vec2 v2f_texCoord;

			out vec4 o_fragColor : FRAG_COLOR;

			uniform float cocScale;
			uniform float focalDistance;
			uniform float noBlurRange;
			uniform float maxBlurRange;
			uniform float zFar;
			uniform vec2 tex0texelSize;
			uniform sampler2D tex0;	// sharp
			uniform sampler2D tex1;	// medium blur
			uniform sampler2D tex2;	// large
			uniform sampler2D tex3;	// depth
			uniform sampler2D tex4;	// blurred coc & blurred depth

			float ComputeCoc(float viewDist) {
				float coc = cocScale * (max(0.0, abs(viewDist - focalDistance) - noBlurRange) / (maxBlurRange - noBlurRange));
				return clamp(coc, 0.0, 1.0);
			}

			vec3 GetSmallBlurSample(sampler2D tex, vec2 st) {
				const float weight = 4.0 / 17.0;
				vec3 sum = weight * tex2D(tex, st + vec2(+0.5, -1.5) * tex0texelSize).rgb;
				sum += weight * tex2D(tex, st + vec2(-1.5, -0.5) * tex0texelSize).rgb;
				sum += weight * tex2D(tex, st + vec2(-0.5, +1.5) * tex0texelSize).rgb;
				sum += weight * tex2D(tex, st + vec2(+1.5, +0.5) * tex0texelSize).rgb;
				return sum;
			}

			void main() {
				float viewDist = tex2D(tex3, v2f_texCoord.st).x * zFar;
				float unblurredCoc = ComputeCoc(viewDist);
				vec2 blurredCoc = tex2D(tex4, v2f_texCoord.st).xw;

				float coc = blurredCoc.x;
				if (blurredCoc.y > depth) {
					coc = unblurredCoc;
				}

				vec3 interpolateColor[4];
				interpolateColor[0] = tex2D(tex0, v2f_texCoord.st).xyz;
				interpolateColor[1] = GetSmallBlurSample(tex0, v2f_texCoord.st);
				interpolateColor[2] = tex2D(tex1, v2f_texCoord.st).xyz;
				interpolateColor[3] = tex2D(tex2, v2f_texCoord.st).xyz;

				interpolateColor[1] += (1.0 / 17.0) * interpolateColor[0];

				float d0 = 0.5;
				float d1 = 0.25;
				float d2 = 0.25;

				// Efficiently calculate the cross-blend weights for each sample.
				// Let the unblurred sample to small blur fade happen over distance
				// d0, the small to medium blur over distance d1, and the medium to
				// large blur over distance d2, where d0 + d1 + d2 = 1
				vec4 dofLerpScale = vec4(-1.0 / d0, -1.0 / d1, -1.0 / d2, 1.0 / d2);
				vec4 dofLerpBias = vec4(1.0, (1.0 - d2) / d1, 1.0 / d2, (d2 - 1.0) / d2);

				vec4 weights = clamp(coc * dofLerpScale + dofLerpBias, 0.0, 1.0);
				weights.yz = min(weights.yz, 1.0 - weights.xy);
				//weights.x = 1.0 - weights.y - weights.z - weights.w;

				//weights = vec4(1.0, 0.0, 0.0, 0.0);

				vec3 color = weights.x * interpolateColor[0] + weights.y * interpolateColor[1] + weights.z * interpolateColor[2] + weights.w * interpolateColor[3];

				o_fragColor = vec4(color, 1.0);
			}
		}
	}*/

	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform vec2 tex0texelSize;
		uniform vec2 tex1texelSize;
		uniform sampler2D tex0;
		uniform sampler2D tex1;

		const vec2 poisson[8] = vec2[](
			vec2(0.0, 0.0),
			vec2(0.527837, -0.085868),
			vec2(-0.040088, 0.536087),
			vec2(-0.670445, -0.179949),
			vec2(-0.419418, -0.616039),
			vec2(0.440453, -0.639399),
			vec2(-0.757088, 0.349334),
			vec2(0.574619, 0.685879)
		);

		void main() {
			vec2 texCoord = v2f_texCoord.st;
			float centerCoc = tex2D(tex0, texCoord).a;

			vec4 color = vec4(0.0, 0.0, 0.0, 0.0);

			float discRadius = centerCoc * 5.0;
			float discRadiusLow = discRadius * 0.3;

			vec4 pixelSize;
			pixelSize.xy = tex0texelSize * discRadius;
			pixelSize.zw = tex1texelSize * discRadiusLow;

			for (int i = 0; i < 8; i++) {
				vec4 tapHigh = tex2D(tex0, texCoord + poisson[i] * pixelSize.xy);
				vec4 tapLow = tex2D(tex1, texCoord + poisson[i] * pixelSize.zw);

				vec4 tap = mix(tapHigh, tapLow, tapHigh.a);
				tap.a = (tap.a >= centerCoc) ? 1.0 : tap.a;

				color.rgb += tap.rgb * tap.a;
				color.a += tap.a;
			}

			color = max(color / color.a, 0.0);

			o_fragColor = color;
		}
	}
}
