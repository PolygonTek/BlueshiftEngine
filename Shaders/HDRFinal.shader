shader "HDRFinal" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec2 in_st : TEXCOORD0;

		out vec2 v2f_texCoord0;
		out vec2 v2f_texCoord1;

		uniform vec2 frameRand;

		void main() {
			gl_Position = in_position;
			v2f_texCoord0.xy = in_st.xy;
			v2f_texCoord1.xy = in_st.xy * vec2(800, 600) / 4.0 + frameRand.xy;
		}
	}

	glsl_fp {
		in vec2 v2f_texCoord0;
		in vec2 v2f_texCoord1;

		out vec4 o_fragColor : FRAG_COLOR;

		#ifdef LOGLUV_HDR
		$include "logluv.glsl"
		#endif
		$include "image_processing.glsl"
		uniform HIGHP sampler2D luminanceMap;
		uniform HIGHP sampler2D colorMap;
		uniform HIGHP sampler2D bloomMap0;
		uniform HIGHP sampler2D bloomMap1;
		uniform HIGHP sampler2D bloomMap2;
		uniform HIGHP sampler2D bloomMap3;
		uniform HIGHP sampler2D bloomMap4;
		uniform sampler2D randomDir4x4Map;
		uniform float middleGray;
		uniform float bloomScale;
		uniform float colorScale;
		uniform vec3 selectiveColor;
		uniform vec4 additiveCmyk;

		const float maxLuminance = 5.0;

		//const vec3 lumVector = vec3(0.2125, 0.7154, 0.0721);
		const vec3 lumVector = vec3(0.299, 0.587, 0.114);

		// vignetting effect (makes corners of image darker)
		float Vignette(vec2 pos, float inner, float outer) {
			float r = length(pos);
			r = 1.0 - smoothstep(inner, outer, r);
			return r;
		}

        vec3 CalcExposedColor(vec3 color, float averageLuminance, float threshold, float keyValue) {
            averageLuminance = max(averageLuminance, 0.0001);

            // Compute current pixel luminance
            float linearExposure = max(keyValue / averageLuminance, 0.0001);
            float exposure = log2(linearExposure);

            //exposure -= threshold;

            return exp2(exposure) * color;
        }

        $include "ToneMapOperators.glsl"

        vec3 ToneMap(vec3 color, float keyValue, float luminanceSaturation, float averageLuminance, float whiteLevel) {
            color = CalcExposedColor(color, averageLuminance, 0.0, keyValue);

            //return color;
            //return ToneMapLogarithmic(color, whiteLevel, luminanceSaturation);
            //return ToneMapExponential(color, whiteLevel, luminanceSaturation);
            //return ToneMapReinhard(color, luminanceSaturation);
            //return ToneMapReinhardExtended(color, whiteLevel, luminanceSaturation);
            //return ToneMapDragoLogarithmic(color, whiteLevel, luminanceSaturation, 0.5);
            return ToneMapFilmicU2(color);
        }
            

	#ifdef COLOR_GRADING
		vec3 SelectiveColor(vec3 color) {
			float colorPickRange = 1.0 - length(color - selectiveColor);

			vec4 cmyk = RGB2CMYK(color);
			cmyk = mix(cmyk, clamp(cmyk + additiveCmyk, vec4(-1.0), vec4(1.0)), colorPickRange);
			return mix(color, CMYK2RGB(cmyk), colorPickRange);
		}
	#endif

		void main() {
		#ifdef LOGLUV_HDR
			float avgLuminance = decodeLogLuv(tex2D(luminanceMap, vec2(0.0, 0.0))).x;
			vec3 sceneColor = decodeLogLuv(tex2D(colorMap, v2f_texCoord0.st));
			vec3 bloomColor = decodeLogLuv(tex2D(bloomMap, v2f_texCoord0.st));
		#else
            float avgLuminance = tex2D(luminanceMap, vec2(0.0, 0.0)).x;
			vec3 sceneColor = tex2D(colorMap, v2f_texCoord0.st).rgb;
			vec3 bloomColor = tex2D(bloomMap0, v2f_texCoord0.st).rgb;
			//bloomColor += tex2D(bloomMap1, v2f_texCoord0.st).rgb;
			//bloomColor += tex2D(bloomMap2, v2f_texCoord0.st).rgb;
			//bloomColor += tex2D(bloomMap3, v2f_texCoord0.st).rgb;
			//bloomColor += tex2D(bloomMap4, v2f_texCoord0.st).rgb;
		#endif

			vec3 color = ToneMap(sceneColor, middleGray, 1.0, avgLuminance, maxLuminance);

			//sceneColor = mix((vec3(0.5,0.5,0.5) + 0.5*tex2D(randomDir4x4Map, v2f_texCoord1.st).xyz) * pixelLuminance * vec3(0.8,0.8,1.8)*2.0, sceneColor, clamp(5.0*pixelLuminance, 0.0, 1.0));
			//sceneColor = mix(pixelLuminance * vec3(0.8,0.8,1.4) * 1.0, sceneColor, clamp(5.0*pixelLuminance, 0.0, 1.0));

		//	float saturation = 0.9; // 1.0 = full saturation, 0.0 = grayscale
		//	float finalLum = dot(color, lumVector);
		//	color.rgb = mix(vec3(finalLum, finalLum, finalLum), color.rgb, saturation);

		//	float invContrast = 1.05; // 2.0 = contrast enhanced, 1.0 = normal contrast, 0.01 = max contrast reduced
		//	color = (color - vec3(0.5, 0.5, 0.5)) * invContrast + vec3(0.5, 0.5, 0.5);

			color *= colorScale;
			color += bloomColor * bloomScale;
			//color = 2.0 * pow(color, vec3(1.5, 1.5, 1.5));

		#ifdef COLOR_GRADING
			color = SelectiveColor(color);
		#endif

			//color *= Vignette(v2f_texCoord0.st * 2.0 - 1.0, 0.8, 1.5);

			o_fragColor = vec4(color, 1.0);
		}
	}
}
