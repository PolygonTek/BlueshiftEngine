//----------------------------------------------------------------------------------
// File:        jni/nv_shader/nv_shader.cpp
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

#include "nv_shader.h"
#include "nv_file/nv_file.h"

void nv_shader_init(AAssetManager* assetManager)
{
    NvFInit(assetManager);
}

char* load_file(const char* file)
{
    NvFile *f = NvFOpen(file);
    size_t size = NvFSize(f);
    char *buffer = new char[size+1];
    NvFRead(buffer, 1, size, f);
    NvFClose(f);
    buffer[size] = '\0';

    return buffer;
}

static int glsl_log(GLuint obj, GLenum check_compile)
{
    if (check_compile == GL_COMPILE_STATUS)
    {
        int len = 0;
        glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &len);
        if(len > 0)
        {
            char *str = (char *) malloc(len * sizeof(char));
            if (str)
            {
                glGetShaderInfoLog(obj, len, NULL, str);
                __android_log_print(ANDROID_LOG_DEBUG, "nv_shader",  "shader_debug: %s\n", str);
                free(str);
            }
        }
    }
    else
    {
        int len = 0;
        glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len);
        if(len > 0)
        {
            char *str = (char *)malloc(len * sizeof(char));
            if (str)
            {
                glGetProgramInfoLog(obj, len, NULL, str);
                __android_log_print(ANDROID_LOG_DEBUG, "nv_shader",  "shader_debug: %s\n", str);
                free(str);
            }
        }
    }
    return 0;
}

GLint nv_load_program(const char *prog, const char *prepend)
{
	int count = 0;
    char* shaders[2];
    int sizes[2];
    char filename[256];

    __android_log_print(ANDROID_LOG_DEBUG, "nv_shader", "in nv_load_program");
    __android_log_print(ANDROID_LOG_DEBUG, "nv_shader", "gl version: %s", glGetString(GL_VERSION));
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);

    if(prepend)
    {
		__android_log_print(ANDROID_LOG_DEBUG, "nv_shader",  "compiling string:\n\"%s\"\n", prepend);
		shaders[count] = (char*)prepend;
		sizes[count] = strlen(shaders[count]);
		count++;
    }

    if(prog)
    {
		sprintf(filename, "%s.vert", prog);
		__android_log_print(ANDROID_LOG_DEBUG, "nv_shader",  "compiling file: %s\n", filename);
		shaders[count] = load_file(filename);
		sizes[count] = strlen(shaders[count]);
		count++;
    }

    if(count == 0)
    	return (GLint)0;

    glShaderSource(vert, count, (const char **) shaders, sizes);
    count--;
    glCompileShader(vert);
    delete[] shaders[count];
    glsl_log(vert, GL_COMPILE_STATUS);

    sprintf(filename, "%s.frag", prog);
    __android_log_print(ANDROID_LOG_DEBUG, "nv_shader",  "compiling file: %s\n", filename);
    shaders[count] = load_file(filename);
    sizes[count] = strlen(shaders[count]);
    count++;

    glShaderSource(frag, count, (const char **) shaders, sizes);
    count--;
    glCompileShader(frag);
    delete[] shaders[count];
    glsl_log(frag, GL_COMPILE_STATUS);

    glLinkProgram(program);
    glsl_log(program, GL_LINK_STATUS);

    return program;
}

GLint nv_load_program_from_strings(const char *vertStr, const char *fragStr)
{
    const char* shaders[2];
    int sizes[2];

    __android_log_print(ANDROID_LOG_DEBUG, "nv_shader", "in nv_load_program_from_strings");
    __android_log_print(ANDROID_LOG_DEBUG, "nv_shader", "gl version: %s", glGetString(GL_VERSION));
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);

    if(vertStr)
    {
		shaders[0] = vertStr;
		sizes[0] = strlen(shaders[0]);
    }
	else
	{
    	return (GLint)0;
	}

    glShaderSource(vert, 1, (const char **) shaders, sizes);
    glCompileShader(vert);
    glsl_log(vert, GL_COMPILE_STATUS);

    if(fragStr)
    {
		shaders[0] = fragStr;
		sizes[0] = strlen(shaders[0]);
    }
	else
	{
    	return (GLint)0;
	}

    glShaderSource(frag, 1, (const char **) shaders, sizes);
    glCompileShader(frag);
    glsl_log(frag, GL_COMPILE_STATUS);

    glLinkProgram(program);
    glsl_log(program, GL_LINK_STATUS);

    return program;
}


static void track_attrib_location(int index)
{
}

void nv_set_attrib_by_name(GLuint prog, const char *name, int size, GLenum type,
        GLboolean normalized, GLsizei stride, void *ptr)
 {
    int index = glGetAttribLocation(prog, name);
    glVertexAttribPointer(index, size, type, normalized, stride, ptr);
    glEnableVertexAttribArray(index);
    track_attrib_location(index);
}

void nv_set_attrib_by_index(GLint index, int size, GLenum type,
        GLboolean normalized, GLsizei stride, void *ptr)
 {
    glVertexAttribPointer(index, size, type, normalized, stride, ptr);
    glEnableVertexAttribArray(index);
    track_attrib_location(index);
}

void nv_flush_tracked_attribs()
{
}


