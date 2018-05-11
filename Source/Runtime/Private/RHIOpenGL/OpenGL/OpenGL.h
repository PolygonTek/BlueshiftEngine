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

    OpenGL

===============================================================================
*/

/*
------------------------------------------------------------------------------
 OpenGL extensions and core versions

 https://en.wikipedia.org/wiki/OpenGL
 https://www.khronos.org/opengl/wiki/History_of_OpenGL
 
 OpenGL 1.2 (1998)
    EXT_texture3D
    EXT_bgra
    EXT_packed_pixels
    EXT_rescale_normal
    EXT_separate_specular_color
    SGIS_texture_edge_clamp
    SGIS_texture_lod
    EXT_draw_range_elements
 
 OpenGL 1.3 (2001)
    ARB_texture_compression
    ARB_texture_cube_map
    ARB_multisample
    ARB_multitexture
    ARB_texture_env_add
    ARB_texture_env_combine
    ARB_texture_env_dot3
    ARB_texture_border_clamp
    ARB_transpose_matrix
 
 OpenGL 1.4 (2002)
    SGIS_generate_mipmap
    NV_blend_square
    EXT_blend_color
    EXT_blend_minmax
    EXT_blend_subtract
    ARB_depth_texture
    ARB_shadow
    EXT_fog_coord
    EXT_multi_draw_arrays
    ARB_point_parameters
    EXT_secondary_color
    EXT_blend_func_separate
    EXT_stencil_wrap
    ARB_texture_env_crossbar
    ARB_texture_mirrored_repeat
    ARB_window_pos
 
 OpenGL 1.5 (2003)
    ARB_vertex_buffer_object
    ARB_occlusion_query
    EXT_shadow_funcs
 
 OpenGL 2.0 (2004)
    GLSL 1.10
    ARB_shading_language_100
    ARB_shader_objects
    ARB_vertex_shader
    ARB_fragment_shader
    ARB_draw_buffers
    ARB_texture_non_power_of_two
    ARB_point_sprite
    EXT_blend_equation_separate
    EXT_stencil_two_side
 
 OpenGL 2.1 (2006)
    GLSL 1.20
    ARB_pixel_buffer_object
    EXT_texture_sRGB
 
 OpenGL 3.0 (2008)
    GLSL 1.30
    EXT_gpu_shader4
    NV_conditional_render
    APPLE_flush_buffer_range
    ARB_color_buffer_float
    NV_depth_buffer_float
    ARB_texture_float
    EXT_packed_float
    EXT_texture_shared_exponent
    EXT_framebuffer_object
    NV_half_float
    ARB_half_float_pixel
    EXT_framebuffer_multisample
    EXT_framebuffer_blit
    EXT_texture_integer
    EXT_texture_array
    EXT_packed_depth_stencil
    EXT_draw_buffers2
    EXT_texture_compression_rgtc
    EXT_transform_feedback
    APPLE_vertex_array_object
    EXT_framebuffer_sRGB
 
 OpenGL 3.1 (2009)
    GLSL 1.40
    ARB_draw_instanced
    ARB_copy_buffer
    NV_primitive_restart
    ARB_texture_buffer_object
    ARB_texture_rectangle
    ARB_uniform_buffer_object
 
 OpenGL 3.2 (2009)
    GLSL 1.50
    ARB_geometry_shader4
    ARB_sync
    ARB_vertex_array_bgra
    ARB_draw_elements_base_vertex
    ARB_fragment_coord_conventions
    ARB_provoking_vertex
    ARB_seamless_cube_map
    ARB_texture_multisample
    ARB_depth_clamp
 
 OpenGL 3.3 (2010)
    GLSL 3.30
    ARB_blend_func_extended
    ARB_explicit_attrib_location
    ARB_occlusion_query2
    ARB_sampler_objects
    ARB_texture_swizzle
    ARB_timer_query
    ARB_instanced_arrays
    ARB_texture_rgb10_a2ui
    ARB_vertex_type_2_10_10_10_rev
 
 OpenGL 4.0 (2010)
    GLSL 4.00
    ARB_draw_buffers_blend
    ARB_draw_indirect
    ARB_gpu_shader5
    ARB_gpu_shader_fp64
    ARB_sample_shading
    ARB_shader_subroutine
    ARB_tessellation_shader
    ARB_texture_buffer_object_rgb32
    ARB_texture_cube_map_array
    ARB_texture_gather
    ARB_texture_query_lod
    ARB_transform_feedback2
 
 OpenGL 4.1 (2010)
    GLSL 4.10
    ARB_debug_output
    ARB_ES2_compatibility
    ARB_get_program_binary
    ARB_separate_shader_objects
    ARB_shader_precision
    ARB_vertex_attrib_64bit
    ARB_viewport_array
 
 OpenGL 4.2 (2011)
    GLSL 4.20
    ARB_base_instance
    ARB_compressed_texture_pixel_storage
    ARB_conservative_depth
    ARB_internalformat_query
    ARB_map_buffer_alignment
    ARB_robustness
    ARB_shader_atomic_counters
    ARB_shader_image_load_store
    ARB_shading_language_420pack
    ARB_texture_compression_bptc
    ARB_texture_storage
    ARB_transform_feedback_instanced

OpenGL 4.3 (2012)
    GLSL 4.30
    KHR_debug
    ARB_arrays_of_arrays
    ARB_clear_buffer_object
    ARB_compute_shader
    ARB_copy_image
    ARB_ES3_compatibility
    ARB_explicit_uniform_location
    ARB_fragment_layer_viewport
    ARB_framebuffer_no_attachments
    ARB_internalformat_query2
    ARB_invalidate_subdata
    ARB_multi_draw_indirect
    ARB_program_interface_query
    ARB_shader_image_size
    ARB_shader_storage_buffer_object
    ARB_stencil_texturing
    ARB_texture_buffer_range
    ARB_texture_query_levels
    ARB_texture_storage_multisample
    ARB_texture_view
    ARB_vertex_attrib_binding
    ARB_robust_buffer_access_behavior
    ARB_robustness_isolation
    WGL_ARB_robustness_isolation
    GLX_ARB_robustness_isolation

OpenGL 4.4 (2013)
    GLSL 4.40
    ARB_buffer_storage
    ARB_clear_texture
    ARB_enhanced_layouts
    ARB_multi_bind
    ARB_query_buffer_object
    ARB_texture_mirror_clamp_to_edge
    ARB_texture_stencil8
    ARB_vertex_type_10f_11f_11f_rev

OpenGL 4.5 (2014)
    GLSL 4.50
    ARB_clip_control
    ARB_cull_distance
    ARB_ES3_1_compatibility
    ARB_conditional_render_inverted
    ARB_derivative_control
    ARB_direct_state_access
    ARB_get_texture_sub_image
    KHR_robustness
    ARB_shader_texture_image_samples
    ARB_texture_barrier

OpenGL 4.6 (2017)
    GLSL 4.60
    GL_ARB_indirect_parameters
    GL_ARB_pipeline_statistics_query
    GL_ARB_polygon_offset_clamp
    GL_KHR_no_error
    GL_ARB_shader_atomic_counter_ops
    GL_ARB_shader_draw_parameters
    GL_ARB_shader_group_vote
    GL_ARB_gl_spirv
    GL_ARB_spirv_extensions
    GL_ARB_texture_filter_anisotropic
    GL_ARB_transform_feedback_overflow_query

------------------------------------------------------------------------------
*/

#if defined(__IOS__)
#include "GGL/ggles3.h"
#elif defined(__ANDROID__)
#include "GGL/ggles3.h"
#include "GGL/gegl.h"
#elif defined(__WIN32__) || defined(__MACOSX__) || defined(__LINUX__)
#include "GGL/gglcore32.h"
    #if defined(__WIN32__)
        #include "GGL/gwgl.h"
    #elif defined(__LINUX__)
        #include "GGL/gglx.h"
    #endif
#endif

#include "Image/Image.h"

BE_NAMESPACE_BEGIN

class OpenGLBase {
public:
    static void             Init();
    
    static bool             SupportsPolygonMode() { return true; }
    static bool             SupportsLineSmooth() { return true; }
    static bool             SupportsTextureBorderColor() { return true; }
    static bool             SupportsTextureLodBias() { return true; }
    static bool             SupportsPackedFloat() { return supportsPackedFloat; }
    static bool             SupportsDepthClamp() { return supportsDepthClamp; }
    static bool             SupportsDepthBoundsTest() { return supportsDepthBoundsTest; }
    static bool             SupportsDepthBufferFloat() { return supportsDepthBufferFloat; }
    static bool             SupportsPixelBufferObject() { return supportsPixelBufferObject; }
    static bool             SupportsFrameBufferSRGB() { return false; }
    static bool             SupportsTextureRectangle() { return supportsTextureRectangle; }
    static bool             SupportsTextureArray() { return supportsTextureArray; }
    static bool             SupportsTextureBufferObject() { return false; }
    static bool             SupportsTextureFilterAnisotropic() { return supportsTextureFilterAnisotropic; }
    static bool             SupportsTextureCompressionS3TC() { return supportsTextureCompressionS3TC; }
    static bool             SupportsTextureCompressionLATC() { return supportsTextureCompressionLATC; }
    static bool             SupportsTextureCompressionETC2() { return supportsTextureCompressionETC2; }
    static bool             SupportsTextureCompressionATC() { return supportsTextureCompressionATC; }
    static bool             SupportsCompressedGenMipmaps() { return false; }
    static bool             SupportsGeometryShader() { return false; }
    static bool             SupportsInstancedArrays() { return false; }
    static bool             SupportsDrawIndirect() { return false; }
    static bool             SupportsMultiDrawIndirect() { return false; }
    static bool             SupportsDebugLabel() { return supportsDebugLabel; }
    static bool             SupportsDebugMarker() { return supportsDebugMarker; }
    static bool             SupportsDebugOutput() { return supportsDebugOutput; }
    static bool             SupportsBufferStorage() { return supportsBufferStorage; }
    static bool             SupportsProgramBinary() { return false; }
    
    static void             PolygonMode(GLenum face, GLenum mode) {}
    static void             ClearDepth(GLdouble depth) {}
    static void             DepthRange(GLdouble znear, GLdouble zfar) {}
    static void             DrawBuffer(GLenum buffer) {}
    static void             BindDefaultFBO() { gglBindFramebuffer(GL_FRAMEBUFFER, 0); }
    static void             VertexAttribDivisor(int index, int divisor) {}
    static void             DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) {}
    static void             DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex) {}
    static void             DrawElementsIndirect(GLenum mode, GLenum type, const void *indirect) {}
    static void             MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride) {}

    static bool             ImageFormatToGLFormat(Image::Format imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal);
    static Image::Format    ToCompressedImageFormat(Image::Format inFormat, bool useNormalMap);
    
private:
    static bool             supportsPackedFloat;
    static bool             supportsDepthClamp;
    static bool             supportsDepthBoundsTest;
    static bool             supportsDepthBufferFloat;
    static bool             supportsPixelBufferObject;
    static bool             supportsTextureRectangle;
    static bool             supportsTextureArray;
    static bool             supportsTextureFilterAnisotropic;
    static bool             supportsTextureCompressionS3TC;
    static bool             supportsTextureCompressionLATC;
    static bool             supportsTextureCompressionRGTC;
    static bool             supportsTextureCompressionETC2;
    static bool             supportsTextureCompressionATC;
    static bool             supportsDebugLabel;
    static bool             supportsDebugMarker;
    static bool             supportsDebugOutput;
    static bool             supportsBufferStorage;
};

BE_NAMESPACE_END
