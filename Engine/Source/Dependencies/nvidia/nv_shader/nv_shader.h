//----------------------------------------------------------------------------------
// File:        jni/nv_shader/nv_shader.h
// SDK Version: v10.14 
// Email:       tegradev@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2007-2012, NVIDIA CORPORATION.  All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA OR ITS SUPPLIERS
// BE  LIABLE  FOR  ANY  SPECIAL,  INCIDENTAL,  INDIRECT,  OR  CONSEQUENTIAL DAMAGES
// WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS OF BUSINESS PROFITS,
// BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
// ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
//
//----------------------------------------------------------------------------------

#ifndef __INCLUDED_NV_SHADER_H
#define __INCLUDED_NV_SHADER_H

#include <GLES3/gl3.h>
#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/asset_manager.h>

/** @file nv_shader.h
  The nv_shader library includes functions to make it easy to load, compile
  and link shader programs in a bound OpenGL ES 2.0 context.  Uses nv_file as
  a part of its implementation.
  @see nv_file.h
  */

/**
  Initializes the library.  Calls NvFInit() internally.
  @see nv_file.h
  @see nv_thread.h
  */
void nv_shader_init(AAssetManager* assetManager);

/**
  Loads a pair of shaders from the given path, using NvFile to find the files.
  Loads .vert for the vertex shader and .frag for the fragment shader.
  @param prog The name (and path) of the shader files, with no extension specified
  @param prepend Optional string to be prepended to the source in both shader files.
  @return nonzero shader program ID in the currently-bound context on success,
  and zero on failure
  */
GLint nv_load_program(const char *prog, const char *prepend = NULL);

/**
  Creates a pair of shaders from the provided string text.
  @param vertStr the complete text of the vertex shader
  @param fragStr the complete text of the fragment shader
  @return nonzero shader program ID in the currently-bound context on success,
  and zero on failure
  */
GLint nv_load_program_from_strings(const char *vertStr, const char *fragStr);

/**
  Finds the given named vertex attribute for the specified program and sets up
  the vertex array for that attrbiute.
  @param prog The program to be queried for index
  @param name The string name of the attribute
  @param size Same use as in the function glVertexAttribPointer
  @param type Same use as in the function glVertexAttribPointer
  @param normalized Same use as in the function glVertexAttribPointer
  @param stride Same use as in the function glVertexAttribPointer
  @param ptr Same use as in the function glVertexAttribPointer
  */
void nv_set_attrib_by_name(GLuint prog, const char *name, int size, GLenum type,
        GLboolean normalized, GLsizei stride, void *ptr);

/**
  Sets up a vertex array for a specified attrbiute
  @param index The index identifying the attribute
  @param size Same use as in the function glVertexAttribPointer
  @param type Same use as in the function glVertexAttribPointer
  @param normalized Same use as in the function glVertexAttribPointer
  @param stride Same use as in the function glVertexAttribPointer
  @param ptr Same use as in the function glVertexAttribPointer
  */
void nv_set_attrib_by_index(GLint index, int size, GLenum type,
        GLboolean normalized, GLsizei stride, void *ptr);

/**
  Disables all vertex attributes.  <b>Currently unimplemented</b>
  */
void nv_flush_tracked_attribs();


#endif
