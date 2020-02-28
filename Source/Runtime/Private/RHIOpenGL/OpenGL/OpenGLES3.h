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

#pragma once

/*
===============================================================================

    OpenGL ES 3.0

    OpenGL 4.3 provides full compatibility with OpenGL ES 3.0.

===============================================================================
*/

/*
------------------------------------------------------------------------------
 OpenGL ES extensions

 https://en.wikipedia.org/wiki/OpenGL_ES

OpenGL ES 1.0
    OES_byte_coordinates
    OES_compressed_paletted_texture
    OES_fixed_point
    OES_query_matrix
    OES_read_format
    OES_single_precision
    OES_compressed_ETC1_RGB8_texture

OpenGL ES 1.1
    OES_draw_texture
    OES_matrix_get
    OES_point_size_array
    OES_point_sprite
    OES_EGL_image
    OES_EGL_image_external
    OES_required_internalformat

OpenGL ES 2.0
    GLSL 1.00
    OES_texture_cube_map
    OES_texture_npot
    OES_depth24
    OES_depth_texture
    OES_element_index_uint
    OES_fbo_render_mipma
    OES_get_program_binary
    OES_mapbuffer
    OES_packed_depth_stencil
    OES_rgb8_rgba8
    OES_stencil8
    OES_vertex_half_float
    OES_EGL_image
    OES_EGL_image_external
    OES_texture_float_linear
    OES_texture_half_float_linear
    OES_texture_float
    OES_texture_half_float
    OES_standard_derivatives
    OES_surfaceless_context
    OES_depth_texture_cube_map
    EXT_texture_filter_anisotropic
    EXT_texture_type_2_10_10_10_REV
    EXT_texture_compression_dxt1
    EXT_texture_format_BGRA8888
    EXT_discard_framebuffer
    EXT_blend_minmax
    EXT_read_format_bgra
    EXT_multi_draw_arrays
    EXT_frag_depth
    EXT_unpack_subimage
    EXT_texture_rg
    EXT_draw_buffers
    EXT_compressed_ETC1_RGB8_sub_texture
    NV_draw_buffers
    NV_fbo_color_attachments
    NV_read_buffer
    NV_read_depth_stencil
    ANGLE_texture_compression_dxt

OpenGL ES 3.0
    GLSL 3.00
    OES_vertex_array_object
    KHR_context_flush_control
    OES_texture_compression_astc
    EXT_texture_border_clamp
    EXT_draw_elements_base_vertex
    OES_EGL_image_external_essl3
    MESA_shader_integer_functions

OpenGL ES 3.1
    GLSL  3.10
    ARB_arrays_of_arrays
    ARB_compute_shader
    ARB_explicit_uniform_location
    ARB_framebuffer_no_attachments
    ARB_program_interface_query
    ARB_shader_atomic_counters
    ARB_shader_image_load_store
    ARB_shader_storage_buffer_object
    ARB_separate_shader_objects
    ARB_stencil_texturing
    ARB_vertex_attrib_binding
    ARB_draw_indirect
    ARB_shading_language_packing
    ARB_shader_image_size
    ARB_texture_storage_multisample
    ARB_texture_multisample
    EXT_shader_integer_mix
    ARB_sample_locations
    OES_texture_view
    NV_image_formats
    EXT_render_snorm
    EXT_texture_norm16

OpenGL ES 3.2
    GLSL  3.20
    KHR_blend_equation_advanced
    EXT_color_buffer_float
    KHR_debug
    KHR_robustness
    OES_copy_image
    OES_draw_buffers_indexed
    OES_draw_elements_base_vertex
    OES_geometry_shader
    OES_gpu_shader5
    OES_sample_shading
    OES_sample_variables
    OES_shader_image_atomic
    OES_shader_io_blocks
    OES_shader_multisample_interpolation
    OES_tessellation_shader
    OES_texture_border_clamp
    OES_texture_buffer
    OES_texture_cube_map_array
    OES_texture_stencil8
    OES_texture_storage_multisample_2d_array
    KHR_texture_compression_astc_ldr
    OES_primitive_bounding_box
    KHR_texture_compression_astc_hdr
    KHR_blend_equation_advanced_coherent
    KHR_texture_compression_astc_sliced_3d
    OES_viewport_array

 ------------------------------------------------------------------------------
*/

#include "OpenGL.h"

#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif

#ifndef GL_POINT
#define GL_POINT 0x1B00
#endif

#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif

#ifndef GL_LINE_SMOOTH
#define GL_LINE_SMOOTH 0x0B20
#endif

#ifndef GL_LINE_SMOOTH_HINT
#define GL_LINE_SMOOTH_HINT 0x0C52
#endif

#ifndef GL_POLYGON_SMOOTH
#define GL_POLYGON_SMOOTH 0x0B41
#endif

#ifndef GL_POLYGON_SMOOTH_HINT
#define GL_POLYGON_SMOOTH_HINT 0x0C53
#endif

#ifndef GL_POLYGON_OFFSET_LINE
#define GL_POLYGON_OFFSET_LINE 0x2A02
#endif

#ifndef GL_POLYGON_OFFSET_POINT
#define GL_POLYGON_OFFSET_POINT 0x2A01
#endif

#ifndef GL_DEPTH_CLAMP
#define GL_DEPTH_CLAMP 0x864F
#endif

#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif

#ifndef GL_TEXTURE_LOD_BIAS
#define GL_TEXTURE_LOD_BIAS 0x8501
#endif

#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE 0x84F5
#endif

#ifndef GL_TEXTURE_BUFFER
#define GL_TEXTURE_BUFFER 0x8C2A
#endif

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY 0x88B9
#endif

#ifndef GL_TEXTURE_CUBE_MAP_ARRAY
#define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
#endif

#ifndef GL_MAX_TEXTURE_BUFFER_SIZE
#define GL_MAX_TEXTURE_BUFFER_SIZE 0x8C2B
#endif

#ifndef GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS 0x8C29
#endif

#ifndef GL_MAX_GEOMETRY_OUTPUT_VERTICES
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES 0x8DE0
#endif

BE_NAMESPACE_BEGIN

class OpenGLES3 : public OpenGLBase {
public:
    static constexpr int    GLSL_VERSION = 300;
    static const char *     GLSL_VERSION_STRING;
    
    static void             Init();
    
    static bool             SupportsPolygonMode() { return false; }
    static bool             SupportsLineSmooth() { return false; }
    static bool             SupportsTextureBorderColor() { return false; }
    static bool             SupportsTextureLodBias() { return false; }
    static bool             SupportsPackedFloat() { return true; }
    static bool             SupportsDepthClamp() { return false; }
    static bool             SupportsDepthBufferFloat() { return true; }
    static bool             SupportsPixelBuffer() { return true; }
    static bool             SupportsDiscardFrameBuffer() { return true; }
    static bool             SupportsFrameBufferSRGB() { return supportsFrameBufferSRGB; }
    static bool             SupportsTextureRectangle() { return true; }
    static bool             SupportsTextureArray() { return true; }
    static bool             SupportsTextureBuffer() { return supportsTextureBuffer; }
    static bool             SupportsTextureCompressionS3TC() { return false; }
    static bool             SupportsTextureCompressionLATC() { return false; }
    static bool             SupportsTextureCompressionRGTC() { return false; }
    static bool             SupportsTextureCompressionETC2() { return true; }
    static bool             SupportsCompressedGenMipmaps() { return false; }
    static bool             SupportsInstancedArrays() { return true; }
    static bool             SupportsDrawIndirect() { return false; }
    static bool             SupportsMultiDrawIndirect() { return false; }
    static bool             SupportsProgramBinary() { return gglProgramBinary != nullptr; }
    static bool             SupportsTimestampQueries() { return supportsTimestampQueries; }

    static void             QueryTimestampCounter(GLuint queryId);

    static void             PolygonMode(GLenum face, GLenum mode) {}
    static void             ClearDepth(GLdouble depth) { gglClearDepthf(depth); }
    static void             DepthRange(GLdouble znear, GLdouble zfar) { gglDepthRangef(znear, zfar); }
    static void             DrawBuffer(GLenum buffer) { gglDrawBuffers(1, &buffer); }
    static void             ReadBuffer(GLenum buffer) { gglReadBuffer(buffer); }
    static void             DrawBuffers(GLsizei count, const GLenum *buffers) { gglDrawBuffers(count, buffers); }
    static void             TexBuffer(GLenum internalFormat, GLuint buffer);
    static void             DiscardFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments) { gglInvalidateFramebuffer(target, numAttachments, attachments); }

    static void             VertexAttribDivisor(int index, int divisor) { gglVertexAttribDivisor(index, divisor); }
    static void             DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
    static void             DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
    static void             DrawElementsIndirect(GLenum mode, GLenum type, const void *indirect);
    static void             MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);

    static void             SetTextureSwizzling(GLenum target, Image::Format::Enum format);
    static bool             ImageFormatToGLFormat(Image::Format::Enum imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal);
    static bool             SupportedImageFormat(Image::Format::Enum imageFormat) { return ImageFormatToGLFormat(imageFormat, false, nullptr, nullptr, nullptr); }
    static Image::Format::Enum ToCompressedImageFormat(Image::Format::Enum inFormat, bool useNormalMap);

private:
    static bool             supportsFrameBufferSRGB;
    static bool             supportsTextureBuffer;
    static bool             supportsTimestampQueries;

    static int              shaderFloatPrecisionLow;
    static int              shaderFloatPrecisionMedium;
    static int              shaderFloatPrecisionHigh;
    static int              shaderIntPrecisionLow;
    static int              shaderIntPrecisionMedium;
    static int              shaderIntPrecisionHigh;
};

BE_NAMESPACE_END
