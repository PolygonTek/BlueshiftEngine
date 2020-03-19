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
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"

#if defined(__IOS__) || defined(__ANDROID__)
    #define IMGUI_IMPL_OPENGL_ES3
#endif

BE_NAMESPACE_BEGIN

// Desktop GL 3.2+ has glDrawElementsBaseVertex() which GL ES and WebGL don't have.
#if defined(IMGUI_IMPL_OPENGL_ES2) || defined(IMGUI_IMPL_OPENGL_ES3) || !defined(GL_VERSION_3_2)
    #define IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET 0
#else
    #define IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET 1
#endif

static GLuint           g_GlVersion = 0;                // Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries.
static char             g_GlslVersionString[32] = "";   // Specified by user or detected based on compile time GL settings.
static GLuint           g_FontTexture = 0;
static GLuint           g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int              g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;                                // Uniforms location
static int              g_AttribLocationVtxPos = 0, g_AttribLocationVtxUV = 0, g_AttribLocationVtxColor = 0; // Vertex attributes location
static unsigned int     g_VboHandle = 0, g_ElementsHandle = 0;

static void ImGui_ImplOpenGL_SetupRenderState(ImDrawData *draw_data, int fb_width, int fb_height, GLuint vertex_array_object) {
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
    gglEnable(GL_BLEND);
    gglBlendEquation(GL_FUNC_ADD);
    gglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gglDisable(GL_CULL_FACE);
    gglDisable(GL_DEPTH_TEST);
    gglEnable(GL_SCISSOR_TEST);
#ifdef GL_POLYGON_MODE
    gglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    gglViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    const float ortho_projection[4][4] = {
        { 2.0f / (R - L),   0.0f,         0.0f,   0.0f },
        { 0.0f,         2.0f / (T - B),   0.0f,   0.0f },
        { 0.0f,         0.0f,        -1.0f,   0.0f },
        { (R + L) / (L - R),  (T + B) / (B - T),  0.0f,   1.0f },
    };
    gglUseProgram(g_ShaderHandle);
    gglUniform1i(g_AttribLocationTex, 0);
    gglUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
#ifdef GL_SAMPLER_BINDING
    gglBindSampler(0, 0); // We use combined texture/sampler state. Applications using GL 3.3 may set that otherwise.
#endif

    (void)vertex_array_object;
#ifndef IMGUI_IMPL_OPENGL_ES2
    gglBindVertexArray(vertex_array_object);
#endif

    // Bind vertex/index buffers and setup attributes for ImDrawVert
    gglBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    gglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
    gglEnableVertexAttribArray(g_AttribLocationVtxPos);
    gglEnableVertexAttribArray(g_AttribLocationVtxUV);
    gglEnableVertexAttribArray(g_AttribLocationVtxColor);
    gglVertexAttribPointer(g_AttribLocationVtxPos, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
    gglVertexAttribPointer(g_AttribLocationVtxUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
    gglVertexAttribPointer(g_AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
}

// OpenGL3 Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so.
void ImGui_ImplOpenGL_RenderDrawData(ImDrawData *draw_data) {
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }

    // Backup GL state
    GLenum last_active_texture; gglGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
    gglActiveTexture(GL_TEXTURE0);
    GLint last_program; gglGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture; gglGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
#ifdef GL_SAMPLER_BINDING
    GLint last_sampler; gglGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
#endif
    GLint last_array_buffer; gglGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
#ifndef IMGUI_IMPL_OPENGL_ES2
    GLint last_vertex_array_object; gglGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array_object);
#endif
#ifdef GL_POLYGON_MODE
    GLint last_polygon_mode[2]; gglGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
#endif
    GLint last_viewport[4]; gglGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; gglGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLenum last_blend_src_rgb; gglGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
    GLenum last_blend_dst_rgb; gglGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
    GLenum last_blend_src_alpha; gglGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
    GLenum last_blend_dst_alpha; gglGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
    GLenum last_blend_equation_rgb; gglGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
    GLenum last_blend_equation_alpha; gglGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
    GLboolean last_enable_blend = gglIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = gglIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = gglIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = gglIsEnabled(GL_SCISSOR_TEST);
    bool clip_origin_lower_left = true;
#if defined(GL_CLIP_ORIGIN) && !defined(__APPLE__)
    GLenum last_clip_origin = 0; gglGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&last_clip_origin); // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)
    if (last_clip_origin == GL_UPPER_LEFT)
        clip_origin_lower_left = false;
#endif

    // Setup desired GL state
    // Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to. VAO are not shared among GL contexts)
    // The renderer would actually work without any VAO bound, but then our VertexAttrib calls would overwrite the default one currently bound.
    GLuint vertex_array_object = 0;
#ifndef IMGUI_IMPL_OPENGL_ES2
    gglGenVertexArrays(1, &vertex_array_object);
#endif
    ImGui_ImplOpenGL_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];

        // Upload vertex/index buffers
        gglBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
        gglBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) {
                    ImGui_ImplOpenGL_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);
                } else {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                    // Apply scissor/clipping rectangle
                    if (clip_origin_lower_left) {
                        gglScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));
                    } else {
                        gglScissor((int)clip_rect.x, (int)clip_rect.y, (int)clip_rect.z, (int)clip_rect.w); // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
                    }

                    // Bind texture, Draw
                    gglBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
#if IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
                    if (g_GlVersion >= 3200) {
                        gglDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)), (GLint)pcmd->VtxOffset);
                    } else
#endif
                    {
                        gglDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
                    }
                }
            }
        }
    }

    // Destroy the temporary VAO
#ifndef IMGUI_IMPL_OPENGL_ES2
    gglDeleteVertexArrays(1, &vertex_array_object);
#endif

    // Restore modified GL state
    gglUseProgram(last_program);
    gglBindTexture(GL_TEXTURE_2D, last_texture);
#ifdef GL_SAMPLER_BINDING
    gglBindSampler(0, last_sampler);
#endif
    gglActiveTexture(last_active_texture);
#ifndef IMGUI_IMPL_OPENGL_ES2
    gglBindVertexArray(last_vertex_array_object);
#endif
    gglBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    gglBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    gglBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend) gglEnable(GL_BLEND); else gglDisable(GL_BLEND);
    if (last_enable_cull_face) gglEnable(GL_CULL_FACE); else gglDisable(GL_CULL_FACE);
    if (last_enable_depth_test) gglEnable(GL_DEPTH_TEST); else gglDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) gglEnable(GL_SCISSOR_TEST); else gglDisable(GL_SCISSOR_TEST);
#ifdef GL_POLYGON_MODE
    gglPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
#endif
    gglViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    gglScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

static void ImGui_ImplOpenGL_DestroyFontsTexture() {
    if (g_FontTexture) {
        ImGuiIO &io = ImGui::GetIO();
        gglDeleteTextures(1, &g_FontTexture);
        io.Fonts->TexID = 0;
        g_FontTexture = 0;
    }
}

bool ImGui_ImplOpenGL_CreateFontsTexture() {
    ImGui_ImplOpenGL_DestroyFontsTexture();

    // Build texture atlas
    ImGuiIO &io = ImGui::GetIO();
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // Upload texture to graphics system
    GLint last_texture;
    gglGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    gglGenTextures(1, &g_FontTexture);
    gglBindTexture(GL_TEXTURE_2D, g_FontTexture);
    gglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    constexpr GLint swiz_a[4] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
    gglTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swiz_a);
#ifdef GL_UNPACK_ROW_LENGTH
    gglPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    gglTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (ImTextureID)(intptr_t)g_FontTexture;

    // Restore state
    gglBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

// If you get an error please report on github. You may try different GL context version or GLSL version. See GL<>GLSL version table at the top of this file.
static bool CheckShader(GLuint handle, const char *desc) {
    GLint status = 0;
    GLint log_length = 0;

    gglGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    gglGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE) {
        BE_ERRLOG("ImGui_ImplOpenGL3_CreateDeviceObjects: failed to compile %s!\n", desc);
    }
    if (log_length > 1) {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        gglGetShaderInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
        BE_ERRLOG("%s\n", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

// If you get an error please report on GitHub. You may try different GL context version or GLSL version.
static bool CheckProgram(GLuint handle, const char* desc) {
    GLint status = 0, log_length = 0;
    gglGetProgramiv(handle, GL_LINK_STATUS, &status);
    gglGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE) {
        BE_ERRLOG("ImGui_ImplOpenGL3_CreateDeviceObjects: failed to link %s! (with GLSL '%s')\n", desc, g_GlslVersionString);
    }
    if (log_length > 1) {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        gglGetProgramInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
        BE_ERRLOG("%s\n", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

static bool ImGui_ImplOpenGL_CreateDeviceObjects() {
    // Backup GL state
    GLint last_texture, last_array_buffer;
    gglGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    gglGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
#ifndef IMGUI_IMPL_OPENGL_ES2
    GLint last_vertex_array;
    gglGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
#endif

    // Parse GLSL version string
    int glsl_version = 130;
    sscanf(g_GlslVersionString, "#version %d", &glsl_version);

    const GLchar *vertex_shader_glsl_120 =
        "uniform mat4 ProjMtx;\n"
        "attribute vec2 Position;\n"
        "attribute vec2 UV;\n"
        "attribute vec4 Color;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *vertex_shader_glsl_130 =
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *vertex_shader_glsl_300_es =
        "precision mediump float;\n"
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *vertex_shader_glsl_410_core =
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *fragment_shader_glsl_120 =
        "#ifdef GL_ES\n"
        "    precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar *fragment_shader_glsl_130 =
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar *fragment_shader_glsl_300_es =
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar *fragment_shader_glsl_410_core =
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D Texture;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    // Select shaders matching our GLSL versions
    const GLchar *vertex_shader = NULL;
    const GLchar *fragment_shader = NULL;
    if (glsl_version < 130) {
        vertex_shader = vertex_shader_glsl_120;
        fragment_shader = fragment_shader_glsl_120;
    } else if (glsl_version >= 410) {
        vertex_shader = vertex_shader_glsl_410_core;
        fragment_shader = fragment_shader_glsl_410_core;
    } else if (glsl_version == 300) {
        vertex_shader = vertex_shader_glsl_300_es;
        fragment_shader = fragment_shader_glsl_300_es;
    } else {
        vertex_shader = vertex_shader_glsl_130;
        fragment_shader = fragment_shader_glsl_130;
    }

    // Create shaders
    const GLchar *vertex_shader_with_version[2] = { g_GlslVersionString, vertex_shader };
    g_VertHandle = gglCreateShader(GL_VERTEX_SHADER);
    gglShaderSource(g_VertHandle, 2, vertex_shader_with_version, NULL);
    gglCompileShader(g_VertHandle);
    CheckShader(g_VertHandle, "vertex shader");

    const GLchar *fragment_shader_with_version[2] = { g_GlslVersionString, fragment_shader };
    g_FragHandle = gglCreateShader(GL_FRAGMENT_SHADER);
    gglShaderSource(g_FragHandle, 2, fragment_shader_with_version, NULL);
    gglCompileShader(g_FragHandle);
    CheckShader(g_FragHandle, "fragment shader");

    g_ShaderHandle = gglCreateProgram();
    gglAttachShader(g_ShaderHandle, g_VertHandle);
    gglAttachShader(g_ShaderHandle, g_FragHandle);
    gglLinkProgram(g_ShaderHandle);
    CheckProgram(g_ShaderHandle, "shader program");

    g_AttribLocationTex = gglGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx = gglGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationVtxPos = gglGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationVtxUV = gglGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationVtxColor = gglGetAttribLocation(g_ShaderHandle, "Color");

    // Create buffers
    gglGenBuffers(1, &g_VboHandle);
    gglGenBuffers(1, &g_ElementsHandle);

    // Restore modified GL state
    gglBindTexture(GL_TEXTURE_2D, last_texture);
    gglBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
#ifndef IMGUI_IMPL_OPENGL_ES2
    gglBindVertexArray(last_vertex_array);
#endif

    return true;
}

static void ImGui_ImplOpenGL_DestroyDeviceObjects() {
    if (g_VboHandle) {
        gglDeleteBuffers(1, &g_VboHandle);
        g_VboHandle = 0;
    }
    if (g_ElementsHandle) {
        gglDeleteBuffers(1, &g_ElementsHandle);
        g_ElementsHandle = 0;
    }
    if (g_ShaderHandle && g_VertHandle) {
        gglDetachShader(g_ShaderHandle, g_VertHandle);
    }
    if (g_ShaderHandle && g_FragHandle) {
        gglDetachShader(g_ShaderHandle, g_FragHandle);
    }
    if (g_VertHandle) {
        gglDeleteShader(g_VertHandle);
        g_VertHandle = 0;
    }
    if (g_FragHandle) {
        gglDeleteShader(g_FragHandle);
        g_FragHandle = 0;
    }
    if (g_ShaderHandle) {
        gglDeleteProgram(g_ShaderHandle);
        g_ShaderHandle = 0;
    }

    ImGui_ImplOpenGL_DestroyFontsTexture();
}

bool ImGui_ImplOpenGL_Init(const char *glsl_version) {
    // Query for GL version
#if !defined(IMGUI_IMPL_OPENGL_ES2)
    GLint major, minor;
    gglGetIntegerv(GL_MAJOR_VERSION, &major);
    gglGetIntegerv(GL_MINOR_VERSION, &minor);
    g_GlVersion = major * 1000 + minor;
#else
    g_GlVersion = 2000; // GLES 2
#endif

    // Setup back-end capabilities flags
    ImGuiIO &io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_opengl3";
#if IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
    if (g_GlVersion >= 3200) {
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    }
#endif

    // Store GLSL version string so we can refer to it later in case we recreate shaders.
    // Note: GLSL version is NOT the same as GL version. Leave this to NULL if unsure.
#if defined(IMGUI_IMPL_OPENGL_ES2)
    if (glsl_version == NULL)
        glsl_version = "#version 100";
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    if (glsl_version == NULL)
        glsl_version = "#version 300 es";
#else
    if (glsl_version == NULL)
        glsl_version = "#version 130";
#endif

    IM_ASSERT((int)strlen(glsl_version) + 2 < IM_ARRAYSIZE(g_GlslVersionString));
    strcpy(g_GlslVersionString, glsl_version);
    strcat(g_GlslVersionString, "\n");

    return true;
}

void ImGui_ImplOpenGL_Shutdown() {
    ImGui_ImplOpenGL_DestroyDeviceObjects();
}

void ImGui_ImplOpenGL_ValidateFrame() {
    if (!g_ShaderHandle) {
        ImGui_ImplOpenGL_CreateDeviceObjects();
    }
    if (!ImGui::GetIO().Fonts->IsBuilt()) {
        ImGui_ImplOpenGL_CreateFontsTexture();
    }
}

BE_NAMESPACE_END
