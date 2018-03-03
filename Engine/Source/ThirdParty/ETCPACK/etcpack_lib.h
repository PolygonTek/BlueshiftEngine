#pragma once

#define ETCPACK_VERSION_S "2.74"

// data types
typedef unsigned char uint8;

namespace etcpack {
    // common
    bool    readCompressParams(void);
    void    setupAlphaTableAndValtab();

    // ETC1 compression 
    double  compressBlockDiffFlipFast(uint8 *img, uint8 *imgdec, int width, int height, int startx, int starty, unsigned int &compressed1, unsigned int &compressed2);
    void    compressBlockDiffFlipFastPerceptual(uint8 *img, uint8 *imgdec, int width, int height, int startx, int starty, unsigned int &compressed1, unsigned int &compressed2);
    void    compressBlockETC1Exhaustive(uint8 *img, uint8 *imgdec, int width, int height, int startx, int starty, unsigned int &compressed1, unsigned int &compressed2);
    void    compressBlockETC1ExhaustivePerceptual(uint8 *img, uint8 *imgdec, int width, int height, int startx, int starty, unsigned int &compressed1, unsigned int &compressed2);
    void    decompressBlockDiffFlipC(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty, int channels);

    // ETC2 compression
    void    compressBlockETC2Fast(uint8 *img, uint8* alphaimg, uint8 *imgdec, int width, int height, int startx, int starty, unsigned int &compressed1, unsigned int &compressed2);
    void    compressBlockETC2RGBA1(uint8 *img, uint8* alphaimg, uint8 *imgdec, int width, int height, int startx, int starty, unsigned int &compressed1, unsigned int &compressed2);
    void    compressBlockETC2Exhaustive(uint8 *img, uint8 *imgdec, int width, int height, int startx, int starty, unsigned int &compressed1, unsigned int &compressed2);
    void    compressBlockETC2FastPerceptual(uint8 *img, uint8 *imgdec, int width, int height, int startx, int starty, unsigned int &compressed1, unsigned int &compressed2);
    void    compressBlockETC2ExhaustivePerceptual(uint8 *img, uint8 *imgdec, int width, int height, int startx, int starty, unsigned int &compressed1, unsigned int &compressed2);
    void    compressBlockAlphaSlow(uint8* data, int ix, int iy, int width, int height, uint8* returnData);
    void    compressBlockAlphaFast(uint8 * data, int ix, int iy, int width, int height, uint8* returnData);
    void    compressBlockAlpha16(uint8* data, int ix, int iy, int width, int height, uint8* returnData);

    void    decompressBlockETC2c(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty, int channels);
    void    decompressBlockETC21BitAlphaC(unsigned int block_part1, unsigned int block_part2, uint8 *img, uint8* alphaimg, int width, int height, int startx, int starty, int channelsRGB);
    void    decompressBlockAlphaC(uint8* data, uint8* img, int width, int height, int ix, int iy, int channels);
    void    decompressBlockAlpha16bitC(uint8* data, uint8* img, int width, int height, int ix, int iy, int channels);
}
