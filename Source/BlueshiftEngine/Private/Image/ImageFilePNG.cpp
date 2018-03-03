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
#include "Core/Heap.h"
#include "Math/Math.h"
#include "File/FileSystem.h"
#include "Image/Image.h"
#include "ImageInternal.h"
#define Z_SOLO
#include "libpng/png.h"
#include "libpng/pngstruct.h"

BE_NAMESPACE_BEGIN

static size_t png_compressed_size;

static void png_read_data(png_structp png, png_bytep data, png_size_t length) {
    memcpy(data, png->io_ptr, length);
    png->io_ptr = (byte *)png->io_ptr + length;
}

bool Image::LoadPNGFromMemory(const char *name, const byte *data, size_t size) {
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, (png_infopp)nullptr, (png_infopp)nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, (png_infopp)nullptr, (png_infopp)nullptr);
        return false;
    }

    png_set_read_fn(png, const_cast<byte *>(data), png_read_data);

    png_read_info(png, info);

    // get picture info
    int	bit_depth;
    int	color_type;
    unsigned int w;
    unsigned int h;
    png_get_IHDR(png, info, (png_uint_32 *)&w, (png_uint_32 *)&h, &bit_depth, &color_type, nullptr, nullptr, nullptr);

    // tell libpng to strip 16 bit/color files down to 8 bits/color
    png_set_strip_16(png);

    // expand paletted images to RGB triplets
    if (color_type & PNG_COLOR_MASK_PALETTE) {
        png_set_expand(png);
    }

    // expand gray-scaled images to RGB triplets
    if (!(color_type & PNG_COLOR_MASK_COLOR)) {
        png_set_gray_to_rgb(png);
    }

    // if there is no alpha information, fill with 255
//	if (!(color_type & PNG_COLOR_MASK_ALPHA)) {
//		png_set_filler(png, 255, PNG_FILLER_AFTER);
    //}

    // expand pictures with less than 8bpp to 8bpp
    if (bit_depth < 8) {
        png_set_packing(png);
    }

    // update structure with the above settings
    png_read_update_info(png, info);

    // allocate the memory to hold the image
    Create2D(w, h, 1, color_type & PNG_COLOR_MASK_ALPHA ? RGBA_8_8_8_8 : RGB_8_8_8, nullptr, 0);
    
    png_bytep *row_pointers = (png_bytep *)Mem_Alloc16(sizeof(png_bytep) * h);

    // set a new exception handler
    if (setjmp(png_jmpbuf(png))) {
        Mem_AlignedFree(row_pointers);
        png_destroy_read_struct(&png, (png_infopp)nullptr, (png_infopp)nullptr);
        return false;
    }

    size_t rowbytes = png_get_rowbytes(png, info);

    for (unsigned int row = 0; row < h; row++) {
        row_pointers[row] = (png_bytep)&(this->pic)[row * rowbytes];
    }

    // read image data
    png_read_image(png, row_pointers);

    // read rest of file, and get additional chunks in info
    png_read_end(png, info);

    // clean up after the read, and free any memory allocated
    png_destroy_read_struct(&png, &info, (png_infopp)nullptr);

    Mem_AlignedFree(row_pointers);

    return true;
}

static void png_write_data(png_structp png, png_bytep data, png_size_t length) {
    //simdProcessor->Memcpy(png->io_ptr, data, length);
    memcpy(png->io_ptr, data, length);
    png->io_ptr = (byte *)png->io_ptr + length;
    png_compressed_size += length;
}

static void png_flush_data(png_structp png) {
}

bool Image::WritePNG(const char *filename) const {
    png_structp	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        return false;
    }

    // Allocate/initialize the image information data.
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, (png_infopp)nullptr);
        return false;
    }

    int	colorType;
    int	bpp;
    Image convertedImage;
    const byte *src = this->pic;

    if (format == RGB_8_8_8) {
        colorType = PNG_COLOR_TYPE_RGB;
        bpp = 3;
    } else if (format == RGBA_8_8_8_8) {
        colorType = PNG_COLOR_TYPE_RGBA;
        bpp = 4;
    } else {
        if (Image::HasAlpha(format)) {
            colorType = PNG_COLOR_TYPE_RGBA;
            bpp = 4;
            if (!ConvertFormat(RGBA_8_8_8_8, convertedImage)) {
                png_destroy_write_struct(&png, (png_infopp)nullptr);
                return false;
            }
        } else {
            colorType = PNG_COLOR_TYPE_RGB;
            bpp = 3;
            if (!ConvertFormat(RGB_8_8_8, convertedImage)) {
                png_destroy_write_struct(&png, (png_infopp)nullptr);
                return false;
            }
        }
        src = convertedImage.pic;
    }

    png_compressed_size = 0;
    byte *buffer = (byte *)Mem_Alloc16(width * height * bpp);

    // Set error handling.
    if (setjmp(png_jmpbuf(png))) {
        Mem_AlignedFree(buffer);
        png_destroy_write_struct(&png, &info);
        return false;
    }

    png_set_write_fn(png, buffer, png_write_data, png_flush_data);
    png_set_IHDR(png, info, width, height, 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // Write the file header information.
    png_write_info(png, info);

    png_bytep *row_pointers = (byte **)Mem_Alloc16(height * sizeof(png_bytep));

    if (setjmp(png_jmpbuf(png))) {
        Mem_AlignedFree(row_pointers);
        Mem_AlignedFree(buffer);
        png_destroy_write_struct(&png, &info);
        return false;
    }

    int row_stride = width * bpp;
    byte *row = (byte *)(src + (0) * row_stride);
    for (int i = 0; i < height; i++) {
        row_pointers[i] = row;
        row += row_stride;
    }

    png_write_image(png, row_pointers);
    png_write_end(png, info);

    // clean up after the write, and free any memory allocated
    png_destroy_write_struct(&png, &info);

    Mem_AlignedFree(row_pointers);

    fileSystem.WriteFile(filename, buffer, (int)png_compressed_size);

    Mem_AlignedFree(buffer);	

    return true;
}

BE_NAMESPACE_END
