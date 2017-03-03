const vec3 RGB2Y   = vec3(+0.29900, +0.58700, +0.11400);
const vec3 RGB2Cb  = vec3(-0.16874, -0.33126, +0.50000);
const vec3 RGB2Cr  = vec3(+0.50000, -0.41869, -0.08131);
const vec3 YCbCr2R = vec3(+1.00000, +0.00000, +1.40200);
const vec3 YCbCr2G = vec3(+1.00000, -0.34414, -0.71414);
const vec3 YCbCr2B = vec3(+1.00000, +1.77200, +0.00000);

vec3 RGB2HSV(const vec3 rgb) {
	float minVal = min(min(rgb.r, rgb.g), rgb.b);
	float maxVal = max(max(rgb.r, rgb.g), rgb.b);
	float delta = maxVal - minVal;
  
	vec3 hsv;  
	hsv.x = 0.0;
	hsv.y = 0.0;
	hsv.z = maxVal;
    
	if (delta != 0.0) {
		// If gray, leave H & S at zero
		hsv.y = delta / maxVal;
		vec3 delRGB;
		delRGB = (((vec3(maxVal, maxVal, maxVal) - rgb) / 6.0) + (delta / 2.0)) / delta;
		if (rgb.x == maxVal) {
			hsv.x = delRGB.z - delRGB.y;
		} else if (rgb.y == maxVal) {
			hsv.x = (1.0/3.0) + delRGB.x - delRGB.z;
		} else if (rgb.z == maxVal) {
			hsv.x = (2.0/3.0) + delRGB.y - delRGB.x;
		}
			
		if (hsv.x < 0.0) {
			hsv.x += 1.0;
		}
		if (hsv.x > 1.0) {
			hsv.x -= 1.0;
		}
	}
    
	return hsv;
}

vec3 HSV2RGB(const vec3 hsv) {
	vec3 rgb = vec3(hsv.z);
	
	if (hsv.y != 0.0) {
		float var_h = hsv.x * 6.0;
		float var_i = floor(var_h);   // Or ... var_i = floor( var_h )
		float var_1 = hsv.z * (1.0 - hsv.y);
		float var_2 = hsv.z * (1.0 - hsv.y * (var_h - var_i));
		float var_3 = hsv.z * (1.0 - hsv.y * (1.0 - (var_h - var_i)));
		
		if (var_i == 0.0) {
			rgb = vec3(hsv.z, var_3, var_1);
		} else if (var_i == 1.0) {
			rgb = vec3(var_2, hsv.z, var_1);
		} else if (var_i == 2.0) {
			rgb = vec3(var_1, hsv.z, var_3);
		} else if (var_i == 3.0) {
			rgb = vec3(var_1, var_2, hsv.z);
		} else if (var_i == 4.0) {
			rgb = vec3(var_3, var_1, hsv.z);
		} else {
			rgb = vec3(hsv.z, var_1, var_2);
		}
	}
	
	return rgb;
}

vec3 RGB2YUV(const vec3 rgb) {
	vec3 yuv;
	yuv.x = dot(rgb, vec3(0.299, 0.587, 0.114));
	yuv.y = (rgb.z - yuv.x) * 0.565;
	yuv.z = (rgb.x - yuv.x) * 0.713;
	return yuv;
}

vec3 YUV2RGB(const vec3 yuv) {
	vec3 rgb;
	rgb.r = yuv.x + 1.403 * yuv.z;
	rgb.g = yuv.x - 0.344 * yuv.y - 1.403 * yuv.z;
	rgb.b = yuv.x + 1.770 * yuv.y;
	return rgb;
}

vec4 RGB2CMYK(const vec3 rgb) {
	vec4 cmyk = vec4(vec3(1.0) - rgb, 0.0);
	cmyk.w = min(min(cmyk.x, cmyk.y), cmyk.z);
	cmyk.xyz = (cmyk.xyz - cmyk.w) / (1.0 - cmyk.w);
	return cmyk;
}

vec3 CMYK2RGB(const vec4 cmyk) {
	vec3 rgb = min(vec3(1.0), cmyk.xyz * (1.0 - cmyk.w) + vec3(cmyk.w));
	return vec3(1.0) - rgb;
}