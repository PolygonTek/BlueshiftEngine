// etcpack_lib.h
#ifndef H_TEX_ETCPACK_LIB_H
#define H_TEX_ETCPACK_LIB_H

#define ETCPACK_VERSION_S "2.74"

// data types
typedef unsigned char uint8;

// common
bool etcpack_readCompressParams(void);
void etcpack_setupAlphaTableAndValtab();

// ETC1 compression 
double etcpack_compressBlockDiffFlipFast(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   etcpack_compressBlockDiffFlipFastPerceptual(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   etcpack_compressBlockETC1Exhaustive(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   etcpack_compressBlockETC1ExhaustivePerceptual(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   etcpack_decompressBlockDiffFlipC(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty, int channels);

// ETC2 compression
void   etcpack_compressBlockETC2Fast(uint8 *img, uint8* alphaimg, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   etcpack_compressBlockETC2RGBA1(uint8 *img, uint8* alphaimg, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   etcpack_compressBlockETC2Exhaustive(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   etcpack_compressBlockETC2FastPerceptual(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   etcpack_compressBlockETC2ExhaustivePerceptual(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   etcpack_compressBlockAlphaSlow(uint8* data, int ix, int iy, int width, int height, uint8* returnData);
void   etcpack_compressBlockAlphaFast(uint8 * data, int ix, int iy, int width, int height, uint8* returnData);
void   etcpack_compressBlockAlpha16(uint8* data, int ix, int iy, int width, int height, uint8* returnData);
void   etcpack_decompressBlockETC2c(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty, int channels);
void   etcpack_decompressBlockETC21BitAlphaC(unsigned int block_part1, unsigned int block_part2, uint8 *img, uint8* alphaimg, int width, int height, int startx, int starty, int channelsRGB);
void   etcpack_decompressBlockAlphaC(uint8* data, uint8* img, int width, int height, int ix, int iy, int channels);
void   etcpack_decompressBlockAlpha16bitC(uint8* data, uint8* img, int width, int height, int ix, int iy, int channels);
#endif
