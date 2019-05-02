shader "HDRFinal" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec2 in_st : TEXCOORD0;

		out vec2 v2f_texCoord0;
		//out vec2 v2f_texCoord1;

		uniform vec2 frameRand;

		void main() {
			gl_Position = in_position;
			v2f_texCoord0.xy = in_st.xy;
			//v2f_texCoord1.xy = in_st.xy * vec2(800, 600) / 4.0 + frameRand.xy;
		}
	}

	glsl_fp {
		in vec2 v2f_texCoord0;
		//in vec2 v2f_texCoord1;

		out vec4 o_fragColor : FRAG_COLOR;

		$include "Colors.glsl"
        $include "ToneMapOperators.glsl"

		uniform HIGHP sampler2D lumaSampler;
		uniform HIGHP sampler2D colorSampler;
		uniform HIGHP sampler2D bloomSampler;
		uniform sampler2D randomDir4x4Sampler;
		uniform float middleGray;
		uniform float bloomScale;
		uniform float colorScale;
		uniform vec3 selectiveColor;
		uniform vec4 additiveCmyk;

		// Vignetting effect (makes corners of image darker)
		float Vignette(vec2 pos, float inner, float outer) {
			float r = length(pos);
			r = 1.0 - smoothstep(inner, outer, r);
			return r;
		}

        float Log2Exposure(float luminance) {
            float linearExposure = max(middleGray / luminance, 0.00001);
            float exposure = log2(linearExposure);

            return exposure;
        }

        vec3 CalcExposedColor(vec3 color, float avgLuminance, float offset) {
            avgLuminance = max(avgLuminance, 0.00001);

            float exposure = Log2Exposure(avgLuminance);
            exposure += offset;

            return exp2(exposure) * color;
        }

        vec3 ToneMap(vec3 color, float avgLuminance) {
            color = CalcExposedColor(color, avgLuminance, 0.0);

        #if TONE_MAPPING_OPERATOR == TONE_MAPPING_LINEAR
            return ToneMapLinear(color);
        #elif TONE_MAPPING_OPERATOR == TONE_MAPPING_EXPONENTIAL
            return ToneMapExponential(color);
        #elif TONE_MAPPING_OPERATOR == TONE_MAPPING_LOGARITHMIC
            return ToneMapLogarithmic(color);
        #elif TONE_MAPPING_OPERATOR == TONE_MAPPING_DRAGO_LOGARITHMIC
            return ToneMapDragoLogarithmic(color, 0.5);
        #elif TONE_MAPPING_OPERATOR == TONE_MAPPING_REINHARD
            return ToneMapReinhard(color);
        #elif TONE_MAPPING_OPERATOR == TONE_MAPPING_REINHARD_EX
            return ToneMapReinhardExtended(color);
        #elif TONE_MAPPING_OPERATOR == TONE_MAPPING_FILMIC_ALU
            return ToneMapFilmicALU(color);
        #elif TONE_MAPPING_OPERATOR == TONE_MAPPING_FILMIC_ACES
            return ToneMapFilmicACES(color);
        #elif TONE_MAPPING_OPERATOR == TONE_MAPPING_FILMIC_UNREAL3
            return ToneMapFilmicUnreal3(color);
        #elif TONE_MAPPING_OPERATOR == TONE_MAPPING_FILMIC_UNCHARTED2
            return ToneMapFilmicUncharted2(color);
        #elif TONE_MAPPING_OPERATOR == TONE_MAPPING_FILMIC_GT
            return ToneMapFilmicGT(color);
        #endif
        }

		vec3 SelectiveColor(vec3 color) {
			float colorPickRange = 1.0 - length(color - selectiveColor);

			vec4 cmyk = RGB_to_CMYK(color);
			cmyk = mix(cmyk, clamp(cmyk + additiveCmyk, vec4(-1.0), vec4(1.0)), colorPickRange);
			return mix(color, CMYK_to_RGB(cmyk), colorPickRange);
		}

		void main() {
            float avgLuminance = tex2D(lumaSampler, vec2(0.0, 0.0)).x;
		#ifdef LOGLUV_HDR
			vec3 sceneColor = decodeLogLuv(tex2D(colorSampler, v2f_texCoord0.st));
			vec3 bloomColor = decodeLogLuv(tex2D(bloomSampler, v2f_texCoord0.st));
		#else
			vec3 sceneColor = tex2D(colorSampler, v2f_texCoord0.st).rgb;
			vec3 bloomColor = tex2D(bloomSampler, v2f_texCoord0.st).rgb;
		#endif

			vec3 color = ToneMap(sceneColor, avgLuminance);

    		color *= colorScale;
			color += bloomColor * bloomScale;

		#ifdef COLOR_GRADING
			color = SelectiveColor(color);
		#endif

			//color *= Vignette(v2f_texCoord0.st * 2.0 - 1.0, 0.8, 1.5);

			o_fragColor = vec4(color, 1.0);
		}
	}
}
