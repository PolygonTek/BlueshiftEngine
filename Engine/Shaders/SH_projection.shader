shader "SH_projection" {
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

		uniform samplerCube radianceCubeMap;
		uniform sampler2D weightMap[6];
		uniform int radianceCubeMapSize;
		uniform float radianceScale;

		void main() {
			vec3 shCoeff = vec3(0.0, 0.0, 0.0);

			float weightMapSize = float(radianceCubeMapSize * 4);

			for (int face = 0; face < 6; face++) {
				for (int y = 0; y < radianceCubeMapSize; y++) {
					for (int x = 0; x < radianceCubeMapSize; x++) {
						int baseS = int(min(floor(v2f_texCoord.s * 4.0), 3.0));
						int baseT = int(min(floor(v2f_texCoord.t * 4.0), 3.0));

						vec2 weightCoord = vec2(float(baseS * radianceCubeMapSize + x) / weightMapSize, float(baseT * radianceCubeMapSize + y) / weightMapSize);
						float weight = tex2D(weightMap[face], weightCoord).x;

						vec3 lightDir = cubeCoord(face, x, y, radianceCubeMapSize);
						vec3 lightColor = texCUBE(radianceCubeMap, lightDir).xyz * radianceScale;

						shCoeff += lightColor * weight;
					}
				}
			}

			o_fragColor = vec4(shCoeff, 1.0);
		}
	}
}
