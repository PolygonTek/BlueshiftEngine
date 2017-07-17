#define SH_CONST1	0.282095
#define SH_CONST2	0.488603
#define SH_CONST3	1.092548
#define SH_CONST4	0.315392
#define SH_CONST5	0.546274
#define SH_CONST6	-0.590044
#define SH_CONST7	2.890611f
#define SH_CONST8	-0.457046
#define SH_CONST9	0.373176

vec3 faceToGLCubeMapCoords(int faceIndex, int x, int y, int cubeMapSize) {
	float s, t, sc, tc;

	s = float(x) / float(cubeMapSize - 1);
	t = float(y) / float(cubeMapSize - 1);
	
	sc = s * 2.0 - 1.0;
	tc = t * 2.0 - 1.0;
	
	vec3 vec;
	
	if (faceIndex == 0) {
		vec = vec3(1.0, -tc, -sc);
	} else if (faceIndex == 1) {
		vec = vec3(-1.0, -tc, sc);
	} else if (faceIndex == 2) {
		vec = vec3(sc, 1.0, tc);
	} else if (faceIndex == 3) {
		vec = vec3(sc, -1.0, -tc);
	} else if (faceIndex == 4) {
		vec = vec3(sc, -tc, 1.0);
	} else {
		vec = vec3(-sc, -tc, -1.0);
	}
	
	vec = normalize(vec);
    return vec;
}
