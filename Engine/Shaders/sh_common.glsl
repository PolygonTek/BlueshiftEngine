#define SH_CONST1	0.282095
#define SH_CONST2	0.488603
#define SH_CONST3	1.092548
#define SH_CONST4	0.315392
#define SH_CONST5	0.546274
#define SH_CONST6	-0.590044
#define SH_CONST7	2.890611f
#define SH_CONST8	-0.457046
#define SH_CONST9	0.373176

vec3 cubeCoord(int face, int x, int y, int cubeMapSize) {
	float s, t, sv, tv;

	s = float(x) / float(cubeMapSize - 1);
	t = float(y) / float(cubeMapSize - 1);
	
	sv = s * 2.0 - 1.0;
	tv = t * 2.0 - 1.0;
	
	vec3 vec;
	
	if (face == 0) {
		vec = vec3(1.0, -tv, -sv);	
	} else if (face == 1) {
		vec = vec3(-1.0, -tv, sv);
	} else if (face == 2) {
		vec = vec3(sv, 1.0, tv);
	} else if (face == 3) {
		vec = vec3(sv, -1.0, -tv);
	} else if (face == 4) {
		vec = vec3(sv, -tv, 1.0);
	} else {
		vec = vec3(-sv, -tv, -1.0);
	}
	
	return normalize(vec);
}
