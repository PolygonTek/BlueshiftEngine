shader "SH_evalIrradianceCubeMap" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec2 in_st : TEXCOORD0;

		out vec2 v2f_texCoord;

		void main() {
			gl_Position = in_position;
			v2f_texCoord = in_st;
		}
	}

	glsl_fp {
		$include "sh_common.glsl"

		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D shCoefficientMap;
		uniform int cubeMapSize;
		uniform int cubeMapFace;
		uniform float reflectanceTable[4];

		void main() {
			vec3 shCoeff[16];

			float t = 0.5 / 4.0;
			for (int y = 0; y < 4; y++) {
				float s = 0.5 / 4.0;
				for (int x = 0; x < 4; x++) {
					shCoeff[y * 4 + x] = tex2D(shCoefficientMap, vec2(s, t)).xyz;
					s += 1.0 / 4.0;
				}
				t += 1.0 / 4.0;
			}

			int x = int(min(floor(v2f_texCoord.x * float(cubeMapSize)), float(cubeMapSize) - 1.0));
			int y = int(min(floor(v2f_texCoord.y * float(cubeMapSize)), float(cubeMapSize) - 1.0));

			vec3 lightDir = cubeCoord(cubeMapFace, x, y, cubeMapSize);

			vec3 color;
			color  = shCoeff[0] * reflectanceTable[0] * SH_CONST1;
			color += shCoeff[1] * reflectanceTable[1] * SH_CONST2 * lightDir.x;
			color += shCoeff[2] * reflectanceTable[1] * SH_CONST2 * lightDir.z;
			color += shCoeff[3] * reflectanceTable[1] * SH_CONST2 * lightDir.y;
			color += shCoeff[4] * reflectanceTable[2] * SH_CONST3 * (lightDir.x * lightDir.y);
			color += shCoeff[5] * reflectanceTable[2] * SH_CONST3 * (lightDir.y * lightDir.z);
			color += shCoeff[6] * reflectanceTable[2] * SH_CONST4 * (3.0 * lightDir.z * lightDir.z - 1.0);
			color += shCoeff[7] * reflectanceTable[2] * SH_CONST3 * (lightDir.x * lightDir.z);
			color += shCoeff[8] * reflectanceTable[2] * SH_CONST5 * (lightDir.x * lightDir.x - lightDir.y * lightDir.y);
			//color += shCoeff[9] * reflectanceTable[3] * SH_CONST6 * lightDir.y * (3.0 * lightDir.x * lightDir.x - lightDir.y * lightDir.y);
			//color += shCoeff[10] * reflectanceTable[3] * SH_CONST7 * lightDir.y * lightDir.x * lightDir.z;
			//color += shCoeff[11] * reflectanceTable[3] * SH_CONST8 * lightDir.y * (-1.0 + 5.0 * lightDir.z * lightDir.z);
			//color += shCoeff[12] * reflectanceTable[3] * SH_CONST9 * lightDir.z * (5.0 * lightDir.z * lightDir.z - 3.0);
			//color += shCoeff[13] * reflectanceTable[3] * SH_CONST8 * lightDir.x * (-1.0 + 5.0 * lightDir.z * lightDir.z);
			//color += shCoeff[14] * reflectanceTable[3] * SH_CONST7 * (lightDir.x * lightDir.x - lightDir.y * lightDir.y) * lightDir.z;
			//color += shCoeff[15] * reflectanceTable[3] * SH_CONST6 * lightDir.x * (lightDir.x * lightDir.x - 3.0 * lightDir.y * lightDir.y);

			o_fragColor = vec4(color, 1.0);
		}
	}
}
