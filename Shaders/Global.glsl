#ifndef GLOBAL_INCLUDED
#define GLOBAL_INCLUDED

#define FLT_SMALLEST        1.192092896e-07 // Smallest positive number, such that 1.0 + FLT_EPSILON != 1.0
#define FLT_MIN             1.175494351e-38 // Minimum representable positive floating-point number
#define FLT_MAX             3.402823466e+38 // Maximum representable floating-point number
#define HALF_MAX            65504.0 // (2 - 2^-10) * 2^15
#define HALF_MAX_MINUS1     65472.0 // (2 - 2^-9) * 2^15
#define HALF_PI             1.5707963267948966192313216916398
#define PI                  3.1415926535897932384626433832795
#define TWO_PI              6.283185307179586476925286766559
#define FOUR_PI             12.566370614359172953850573533118
#define INV_HALF_PI         0.63661977236758134307553505349004
#define INV_PI              0.31830988618379067153776752674503
#define INV_TWO_PI          0.15915494309189533576888376337251
#define INV_FOUR_PI         0.07957747154594766788444188168626

float pow4(float f) {
    float f2 = f * f;
    return f2 * f2;
}

float pow5(float f) {
    float f2 = f * f;
    return f2 * f2 * f;
}

float log10(float x) {
    // 1.0 / log(10) = 0.4342944819
    return 0.4342944819 * log(x);
}

#endif