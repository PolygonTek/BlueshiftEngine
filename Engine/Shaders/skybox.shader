shader "skybox" {
	glsl_vp {
		in vec4 in_position : POSITION;
		in vec4 in_color : COLOR;

		out vec4 v2f_color;
		out vec3 v2f_texCoord;

		uniform mat4 modelViewMatrix;
		uniform mat4 modelViewProjectionMatrix;

		#ifdef INSTANCED_DRAW
			struct entityData_t {
				vec4 worldMatrixS;
				vec4 worldMatrixT;
				vec4 worldMatrixR;
				vec3 localViewOrigin;
				mat3 localLightAxis;
				vec4 localLightOrigin;
				vec4 textureMatrixS;
				vec4 textureMatrixT;
				vec4 vertexColorScale;
				vec4 vertexColorAdd;
				vec4 constantColor;
			};

			uniform entityData_t entityData[256];
			#define ED entityData[gl_InstanceIDARB]
		#else
			uniform vec4 worldMatrixS;
			uniform vec4 worldMatrixT;
			uniform vec4 worldMatrixR;
			uniform vec3 localViewOrigin;
			uniform mat3 localLightAxis;
			uniform vec4 localLightOrigin;
			uniform vec4 textureMatrixS;
			uniform vec4 textureMatrixT;
			uniform vec4 vertexColorScale;
			uniform vec4 vertexColorAdd;
			uniform vec4 constantColor;
		#endif

		uniform float rotationZ;

		void main() {
			vec3 v = in_position.xyz + modelViewMatrix[3].xyz * mat3(modelViewMatrix);

			float s = sin(rotationZ);
			float c = cos(rotationZ);

			mat3 rotMat;
			rotMat[0] = vec3(c, s, 0.0);
			rotMat[1] = vec3(-s, c, 0.0);
			rotMat[2] = vec3(0.0, 0.0, 1.0);
			v = v * rotMat;

			v2f_texCoord.x = -v.y;
			v2f_texCoord.y = +v.z;
			v2f_texCoord.z = -v.x;

			v2f_color = (in_color * vertexColorScale + vertexColorAdd) * constantColor;

			gl_Position = modelViewProjectionMatrix * in_position;
		}
	}

	glsl_fp {
		in vec4 v2f_color;
		in vec3 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform samplerCube skyCubeMap;

		void main() {
			vec3 skycolor = texCUBE(skyCubeMap, v2f_texCoord).xyz;
			o_fragColor = v2f_color * vec4(skycolor, 1.0);
		}
	}
}
