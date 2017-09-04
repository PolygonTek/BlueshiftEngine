shader "sunShaftsGen" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec2 in_st : TEXCOORD0;

		out vec2 v2f_texCoord;
		out vec4 v2f_sunPos;

		uniform vec3 worldSunPos;
		uniform mat4 viewProjectionMatrix;

		void main() {
			vec4 sunPosH = viewProjectionMatrix * vec4(worldSunPos, 1.0);
			v2f_sunPos = sunPosH;
			v2f_sunPos.xy = (sunPosH.xy + sunPosH.ww) * 0.5;
			v2f_sunPos.z = dot(vec3(viewProjectionMatrix[0][2], viewProjectionMatrix[1][2], viewProjectionMatrix[2][2]), normalize(worldSunPos)); // sign

			gl_Position = in_position;
			v2f_texCoord = in_st;
		}
	}

	glsl_fp {
		in vec2 v2f_texCoord;
		in vec4 v2f_sunPos;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D sunShaftsMap;
		uniform float shaftScale;

		void main() {
			float sign = v2f_sunPos.z;
			vec2 screenSunPos = v2f_sunPos.xy / v2f_sunPos.w;
			vec2 texCoord = v2f_texCoord.st;

			vec2 sunVec = screenSunPos - texCoord;

			float sunDist = saturate(sign) * (1.0 - saturate(dot(sunVec, sunVec) * 1.0));

			sunVec *= shaftScale * sign;

			vec4 accum = tex2D(sunShaftsMap, texCoord);
			texCoord += sunVec;
			accum += tex2D(sunShaftsMap, texCoord) * (1.0 - 1.0 / 8.0);
			texCoord += sunVec;
			accum += tex2D(sunShaftsMap, texCoord) * (1.0 - 2.0 / 8.0);
			texCoord += sunVec;
			accum += tex2D(sunShaftsMap, texCoord) * (1.0 - 3.0 / 8.0);
			texCoord += sunVec;
			accum += tex2D(sunShaftsMap, texCoord) * (1.0 - 4.0 / 8.0);
			texCoord += sunVec;
			accum += tex2D(sunShaftsMap, texCoord) * (1.0 - 5.0 / 8.0);
			texCoord += sunVec;
			accum += tex2D(sunShaftsMap, texCoord) * (1.0 - 6.0 / 8.0);
			texCoord += sunVec;
			accum += tex2D(sunShaftsMap, texCoord) * (1.0 - 7.0 / 8.0);
			texCoord += sunVec;

			accum /= 4.5;

			o_fragColor = accum * sunDist;
		}
	}
}
