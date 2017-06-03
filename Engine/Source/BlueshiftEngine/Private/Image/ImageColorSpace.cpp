// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    RGB <-> YCoCg

    Y  = [ 1/4  1/2  1/4] [R]
    Co = [ 1/2    0 -1/2] [G]
    Cg = [-1/4  1/2 -1/4] [B]

    R  = [   1    1   -1] [Y]
    G  = [   1    0    1] [Co]
    B  = [   1   -1   -1] [Cg]

    YCoCgA(4:2:0) 에 데이터가 담기는 순서
    
    YYYYYYYY... (8x8) * (2*2) 0
    CoCoCoCoCoCoCo... (8x8) 256
    CgCgCgCgCgCgCg... (8x8) 320
    AAAAAAAA... (8x8) * (2*2) 384 (alpha 가 있을 경우에만)

-------------------------------------------------------------------------------
*/

#define RGB_TO_YCOCG_Y(r, g, b)   ((( r +   (g<<1) +  b    ) + 2) >> 2)
#define RGB_TO_YCOCG_CO(r, g, b)  ((((r<<1)        - (b<<1)) + 2) >> 2)
#define RGB_TO_YCOCG_CG(r, g, b)  (((-r +   (g<<1) -  b    ) + 2) >> 2)

#define COCG_TO_R(co, cg)         (co - cg)
#define COCG_TO_G(co, cg)         (cg)
#define COCG_TO_B(co, cg)         (-co - cg)

static void ConvertRGBAToYCoCgA(byte *image, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        int r = image[i*4+0];
        int g = image[i*4+1];
        int b = image[i*4+2];
        int a = image[i*4+3];

        image[i*4+0] = ClampByte(RGB_TO_YCOCG_Y(r, g, b));
        image[i*4+1] = ClampByte(RGB_TO_YCOCG_CO(r, g, b) + 128);
        image[i*4+2] = ClampByte(RGB_TO_YCOCG_CG(r, g, b) + 128);
        image[i*4+3] = a;
    }
}

static void ConvertYCoCgAToRGBA(byte *image, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        int y  = image[i*4+0];
        int co = image[i*4+1] - 128;
        int cg = image[i*4+2] - 128;
        int a  = image[i*4+3];

        image[i*4+0] = ClampByte(y + COCG_TO_R(co, cg));
        image[i*4+1] = ClampByte(y + COCG_TO_G(co, cg));
        image[i*4+2] = ClampByte(y + COCG_TO_B(co, cg));
        image[i*4+3] = a;
    }
}

void RGBToYCoCg(short *YCoCg, const byte *rgb, int stride) {
    int c[5][3];
    int i, j, k;

    for (k = 0; k < 4; k++) {
        const byte *pByte = rgb + ((k & 2) * stride + (k & 1) * 3 * 2) * 4;

        short *py = YCoCg + k * 64;

        for (j = 0; j < 8; j++) {
            for (i = 0; i < 2; i++) {
                c[0][0] = pByte[i*12+0];
                c[0][1] = pByte[i*12+1];
                c[0][2] = pByte[i*12+2];

                c[1][0] = pByte[i*12+3];
                c[1][1] = pByte[i*12+4];
                c[1][2] = pByte[i*12+5];

                c[2][0] = pByte[i*12+6];
                c[2][1] = pByte[i*12+7];
                c[2][2] = pByte[i*12+8];

                c[3][0] = pByte[i*12+9];
                c[3][1] = pByte[i*12+10];
                c[3][2] = pByte[i*12+11];

                py[j*8+i*4+0] = ClampShort(RGB_TO_YCOCG_Y(c[0][0], c[0][1], c[0][2]) - 128);
                py[j*8+i*4+1] = ClampShort(RGB_TO_YCOCG_Y(c[1][0], c[1][1], c[1][2]) - 128);
                py[j*8+i*4+2] = ClampShort(RGB_TO_YCOCG_Y(c[2][0], c[2][1], c[2][2]) - 128);
                py[j*8+i*4+3] = ClampShort(RGB_TO_YCOCG_Y(c[3][0], c[3][1], c[3][2]) - 128);
            }
                
            pByte += stride;
        }
    }	

    const byte *pByte = rgb;

    for (j = 0; j < 8; j++) {
        short *pc = YCoCg + 256 + j * 8;

        for (i = 0; i < 8; i++) {
            c[0][0] = pByte[0];
            c[0][1] = pByte[1];
            c[0][2] = pByte[2];

            c[1][0] = pByte[3];
            c[1][1] = pByte[4];
            c[1][2] = pByte[5];

            pByte += stride;

            c[2][0] = pByte[0];
            c[2][1] = pByte[1];
            c[2][2] = pByte[2];

            c[3][0] = pByte[3];
            c[3][1] = pByte[4];
            c[3][2] = pByte[5];

            pByte -= stride;

            c[4][0] = (c[0][0] + c[1][0] + c[2][0] + c[3][0]) >> 2;
            c[4][1] = (c[0][1] + c[1][1] + c[2][1] + c[3][1]) >> 2;
            c[4][2] = (c[0][2] + c[1][2] + c[2][2] + c[3][2]) >> 2;

            pc[i+0]  = ClampShort(RGB_TO_YCOCG_CO(c[4][0], c[4][1], c[4][2]));
            pc[i+64] = ClampShort(RGB_TO_YCOCG_CG(c[4][0], c[4][1], c[4][2]));

            pByte += 2 * 3;
        }

        pByte -= 2 * 3 * 8;
        pByte += stride * 2;
    }
}

void RGBAToYCoCgA(short *YCoCgA, const byte *rgba, int stride) {
    int c[5][3];
    int i, j, k;

    for (k = 0; k < 4; k++) {
        const byte *pByte = rgba + ((k & 2) * stride + (k & 1) * 4 * 2) * 4;

        short *py = YCoCgA + k * 64;
        short *pa = YCoCgA + 256 + 64 * 2 + k * 64;

        for (j = 0; j < 8; j++) {
            for (i = 0; i < 2; i++) {
                c[0][0] = pByte[i*12+0];    // Red
                c[0][1] = pByte[i*12+1];    // Green
                c[0][2] = pByte[i*12+2];    // Blue

                c[1][0] = pByte[i*12+4];    // Red
                c[1][1] = pByte[i*12+5];    // Green
                c[1][2] = pByte[i*12+6];    // Blue

                c[2][0] = pByte[i*12+8];    // Red
                c[2][1] = pByte[i*12+9];    // Green
                c[2][2] = pByte[i*12+10];   // Blue
                
                c[3][0] = pByte[i*12+12];   // Red
                c[3][1] = pByte[i*12+13];   // Green
                c[3][2] = pByte[i*12+14];   // Blue
                
                py[j*8+i*4+0] = ClampShort(RGB_TO_YCOCG_Y(c[0][0], c[0][1], c[0][2]) - 128);
                py[j*8+i*4+1] = ClampShort(RGB_TO_YCOCG_Y(c[1][0], c[1][1], c[1][2]) - 128);
                py[j*8+i*4+2] = ClampShort(RGB_TO_YCOCG_Y(c[2][0], c[2][1], c[2][2]) - 128);
                py[j*8+i*4+3] = ClampShort(RGB_TO_YCOCG_Y(c[3][0], c[3][1], c[3][2]) - 128);

                pa[j*8+i*4+0] = pByte[i*12+3] - 128;    // Alpha
                pa[j*8+i*4+1] = pByte[i*12+7] - 128;    // Alpha
                pa[j*8+i*4+2] = pByte[i*12+11] - 128;   // Alpha
                pa[j*8+i*4+3] = pByte[i*12+15] - 128;   // Alpha
            }		

            pByte += stride;
        }
    }	

    const byte *pByte = rgba;

    for (j = 0; j < 8; j++) {
        short *pc = YCoCgA + 256 + j * 8;

        for (i = 0; i < 8; i++) {
            c[0][0] = pByte[0]; // Red
            c[0][1] = pByte[1]; // Green
            c[0][2] = pByte[2]; // Blue

            c[1][0] = pByte[4]; // Red
            c[1][1] = pByte[5]; // Green
            c[1][2] = pByte[6]; // Blue

            pByte += stride;

            c[2][0] = pByte[0]; // Red
            c[2][1] = pByte[1]; // Green
            c[2][2] = pByte[2]; // Blue

            c[3][0] = pByte[4]; // Red
            c[3][1] = pByte[5]; // Green
            c[3][2] = pByte[6]; // Blue

            pByte -= stride;

            c[4][0] = (c[0][0] + c[1][0] + c[2][0] + c[3][0]) >> 2;
            c[4][1] = (c[0][1] + c[1][1] + c[2][1] + c[3][1]) >> 2;
            c[4][2] = (c[0][2] + c[1][2] + c[2][2] + c[3][2]) >> 2;

            pc[i+0]  = ClampShort(RGB_TO_YCOCG_CO(c[4][0], c[4][1], c[4][2]));
            pc[i+64] = ClampShort(RGB_TO_YCOCG_CG(c[4][0], c[4][1], c[4][2]));

            pByte += 2 * 4;
        }

        pByte -= 2 * 4 * 8;
        pByte += stride * 2;
    }
}

void YCoCgToRGB(byte *rgb, int stride, const short *YCoCg) {
    int i, j, k;

    // writes out one 8*8 block of the 16*16 tile per iteration
    for (k = 0; k < 4; k++) {
        byte *pByte = rgb + ((k & 2) * stride + (k & 1) * (3 * 2)) * 4;

        const short *py = YCoCg + k * 64;
        const short *pc = YCoCg + 256 + ((k & 2) * 4 + (k & 1)) * 4;

        // writes out 2 rows of an 8*8 block per iteration
        for (j = 0; j < 4; j++) {
            for (i = 0; i < 4; i++) {
                int y, co, cg, r, s, t;

                co = pc[i+0];
                cg = pc[i+64];

                r = co - cg;
                s = cg;
                t = co + cg;

                y = py[i*2+0+0+0] + 128;
                pByte[i*2*3+0*3+0] = ClampByte(y+r); // Red
                pByte[i*2*3+0*3+1] = ClampByte(y+s); // Green
                pByte[i*2*3+0*3+2] = ClampByte(y-t); // Blue

                y = py[i*2+0+1+0] + 128;
                pByte[i*2*3+1*3+0] = ClampByte(y+r); // Red
                pByte[i*2*3+1*3+1] = ClampByte(y+s); // Green
                pByte[i*2*3+1*3+2] = ClampByte(y-t); // Blue

                pByte += stride;

                y = py[i*2+8+0+0] + 128;
                pByte[i*2*3+0*3+0] = ClampByte(y+r); // Red
                pByte[i*2*3+0*3+1] = ClampByte(y+s); // Green
                pByte[i*2*3+0*3+2] = ClampByte(y-t); // Blue

                y = py[i*2+8+1+0] + 128;
                pByte[i*2*3+1*3+0] = ClampByte(y+r); // Red
                pByte[i*2*3+1*3+1] = ClampByte(y+s); // Green
                pByte[i*2*3+1*3+2] = ClampByte(y-t); // Blue

                pByte -= stride;
            }

            py += 16;
            pc += 8;

            pByte += 2 * stride;
        }
    }
}

void YCoCgAToRGBA(byte *rgba, int stride, const short *YCoCgA) {
    int i, j, k;

    // writes out one 8*8 block of the 16*16 tile per iteration
    for (k = 0; k < 4; k++) {
        byte *pByte = rgba + ((k & 2) * stride + (k & 1) * (4 * 2)) * 4;

        const short *py = YCoCgA + k * 64;
        const short *pc = YCoCgA + 256 + ((k & 2) * 4 + (k & 1)) * 4;

        // writes out 2 rows of an 8*8 block per iteration
        for (j = 0; j < 4; j++) {
            for (i = 0; i < 4; i++) {
                int y, co, cg, r, s, t, a;

                co = pc[i+0];
                cg = pc[i+64];

                r = co - cg;
                s = cg;
                t = co + cg;

                y = py[i*2+0+0+0] + 128;
                a = py[i*2+0+0+384] + 128;
                pByte[i*2*4+0*4+0] = ClampByte(y+r); // Red
                pByte[i*2*4+0*4+1] = ClampByte(y+s); // Green
                pByte[i*2*4+0*4+2] = ClampByte(y-t); // Blue
                pByte[i*2*4+0*4+3] = ClampByte(a); // Alpha

                y = py[i*2+0+1+0] + 128;
                a = py[i*2+0+1+384] + 128;
                pByte[i*2*4+1*4+0] = ClampByte(y+r); // Red
                pByte[i*2*4+1*4+1] = ClampByte(y+s); // Green
                pByte[i*2*4+1*4+2] = ClampByte(y-t); // Blue
                pByte[i*2*4+1*4+3] = ClampByte(a); // Alpha

                pByte += stride;

                y = py[i*2+8+0+0] + 128;
                a = py[i*2+8+0+384] + 128;
                pByte[i*2*4+0*4+0] = ClampByte(y+r); // Red
                pByte[i*2*4+0*4+1] = ClampByte(y+s); // Green
                pByte[i*2*4+0*4+2] = ClampByte(y-t); // Blue
                pByte[i*2*4+0*4+3] = ClampByte(a); // Alpha

                y = py[i*2+8+1+0] + 128;
                a = py[i*2+8+1+384] + 128;
                pByte[i*2*4+1*4+0] = ClampByte(y+r); // Red
                pByte[i*2*4+1*4+1] = ClampByte(y+s); // Green
                pByte[i*2*4+1*4+2] = ClampByte(y-t); // Blue
                pByte[i*2*4+1*4+3] = ClampByte(a); // Alpha

                pByte -= stride;
            }

            py += 16;
            pc += 8;

            pByte += 2 * stride;
        }
    }
}

BE_NAMESPACE_END