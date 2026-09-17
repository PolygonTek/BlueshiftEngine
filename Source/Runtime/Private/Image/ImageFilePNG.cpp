// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Core/Heap.h"
#include "Math/Math.h"
#include "IO/FileSystem.h"
#include "Image/Image.h"
#include "ImageInternal.h"
#define Z_SOLO
//#include "libpng/pngstruct.h"
#include "libpng/png.h"

BE_NAMESPACE_BEGIN

struct PNGReadContext {
    const byte *current;
    size_t remaining;
};

static void PNGCBAPI png_read_error(png_structp png, png_const_charp message) {
    const char *name = static_cast<const char *>(png_get_error_ptr(png));
    BE_WARNLOG("Image::LoadPNGFromMemory: '%s': %s\n", name ? name : "<memory>", message);
    png_longjmp(png, 1);
}

static void PNGCBAPI png_read_data(png_structp png, png_bytep data, png_size_t length) {
    PNGReadContext *ctx = static_cast<PNGReadContext *>(png_get_io_ptr(png));
    if (length > ctx->remaining) {
        png_error(png, "Truncated PNG data");
    }
    memcpy(data, ctx->current, length);
    ctx->current += length;
    ctx->remaining -= length;
}

bool Image::LoadPNGFromMemory(const char *name, const byte *data, size_t size) {
    if (!data || !size) {
        BE_WARNLOG("Image::LoadPNGFromMemory: '%s': Empty PNG data\n", name ? name : "<memory>");
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, const_cast<char *>(name), png_read_error, nullptr);
    if (!png) {
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, (png_infopp)nullptr, (png_infopp)nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, (png_infopp)nullptr);
        return false;
    }

    PNGReadContext ctx;
    ctx.current = data;
    ctx.remaining = size;
    png_set_read_fn(png, &ctx, png_read_data);

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

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }

    // expand gray-scaled images to RGB triplets
    if (!(color_type & PNG_COLOR_MASK_COLOR)) {
        png_set_gray_to_rgb(png);
    }

    // if there is no alpha information, fill with 255
    /*if (!(color_type & PNG_COLOR_MASK_ALPHA)) {
        png_set_filler(png, 255, PNG_FILLER_AFTER);
    }*/

    // expand pictures with less than 8bpp to 8bpp
    if (bit_depth < 8) {
        png_set_packing(png);
    }

    // update structure with the above settings
    png_read_update_info(png, info);

    png_get_IHDR(png, info, (png_uint_32 *)&w, (png_uint_32 *)&h, &bit_depth, &color_type, nullptr, nullptr, nullptr);

    const size_t rowbytes = png_get_rowbytes(png, info);
    const int channels = (color_type & PNG_COLOR_MASK_ALPHA) ? 4 : 3;
    if (!w || !h || w > INT32_MAX || h > INT32_MAX || bit_depth != 8 ||
        rowbytes != static_cast<uint64_t>(w) * channels ||
        rowbytes > SIZE_MAX / h || h > SIZE_MAX / sizeof(png_bytep)) {
        png_error(png, "Invalid PNG output size");
    }

    // allocate the memory to hold the image
    Create2D(w, h, 1, (color_type & PNG_COLOR_MASK_ALPHA) ? Format::R8G8B8A8 : Format::R8G8B8, GammaSpace::sRGB, nullptr, Flag::None);
    
    png_bytep *row_pointers = (png_bytep *)Mem_Alloc16(sizeof(png_bytep) * h);

    // set a new exception handler
    if (setjmp(png_jmpbuf(png))) {
        Mem_AlignedFree(row_pointers);
        png_destroy_read_struct(&png, &info, (png_infopp)nullptr);
        Clear();
        return false;
    }

    if (!pic || !row_pointers) {
        png_error(png, "Could not allocate PNG image data");
    }

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

struct PNGWriteContext {
    File *file;
};

static void png_write_data(png_structp png, png_bytep data, png_size_t length) {
    auto *ctx = static_cast<PNGWriteContext *>(png_get_io_ptr(png));
    if (!ctx || !ctx->file || !ctx->file->Write(data, length)) {
        png_error(png, "PNG write failed");
        return;
    }
}

static void png_flush_data(png_structp png) {
}

bool Image::WritePNG(File *fp) const {
    if (!fp) {
        return false;
    }

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

    if (format == Format::R8G8B8) {
        colorType = PNG_COLOR_TYPE_RGB;
        bpp = 3;
    } else if (format == Format::R8G8B8A8) {
        colorType = PNG_COLOR_TYPE_RGBA;
        bpp = 4;
    } else {
        if (Image::HasAlpha(format)) {
            colorType = PNG_COLOR_TYPE_RGBA;
            bpp = 4;
            if (!ConvertFormat(Format::R8G8B8A8, convertedImage)) {
                png_destroy_write_struct(&png, &info);
                return false;
            }
        } else {
            colorType = PNG_COLOR_TYPE_RGB;
            bpp = 3;
            if (!ConvertFormat(Format::R8G8B8, convertedImage)) {
                png_destroy_write_struct(&png, &info);
                return false;
            }
        }
        src = convertedImage.pic;
    }

    png_bytep *row_pointers = (byte **)Mem_Alloc16(height * sizeof(png_bytep));

    // Set error handling.
    if (setjmp(png_jmpbuf(png))) {
        Mem_AlignedFree(row_pointers);
        png_destroy_write_struct(&png, &info);
        return false;
    }

    PNGWriteContext ctx;
    ctx.file = fp;
    png_set_write_fn(png, &ctx, png_write_data, png_flush_data);
    png_set_IHDR(png, info, width, height, 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // Write the file header information.
    png_write_info(png, info);

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

    return true;
}

bool Image::WritePNG(const char *filename) const {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Image::WritePNG: failed to open file '%s' for writing\n", filename);
        return false;
    }

    bool ret = WritePNG(fp);

    fileSystem.CloseFile(fp);

    return ret;
}

BE_NAMESPACE_END
