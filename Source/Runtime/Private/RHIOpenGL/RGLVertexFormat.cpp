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

BE_NAMESPACE_BEGIN

static const GLenum toGLType[] = {
    GL_BYTE,
    GL_UNSIGNED_BYTE,
    GL_INT,
    GL_UNSIGNED_INT,
    GL_FLOAT,
    GL_HALF_FLOAT,
};

static const int GLTypeSize[] = { 
    sizeof(GLbyte),
    sizeof(GLubyte),
    sizeof(GLint),
    sizeof(GLuint),
    sizeof(GLfloat),
    sizeof(GLhalf)
};

RHI::Handle OpenGLRHI::CreateVertexFormat(int numElements, const VertexElement *elements) {
    GLVertexFormat *vertexFormat = new GLVertexFormat;
    
    memset(vertexFormat->vertexSize, 0, sizeof(vertexFormat->vertexSize));
    memset(vertexFormat->vertexElements, 0, sizeof(vertexFormat->vertexElements));

    for (int i = 0; i < numElements; i++) {
        const VertexElement *element = &elements[i];

        int stream = element->stream;

        GLVertexElementInternal *ve = &vertexFormat->vertexElements[element->usage];
        ve->stream = stream;
        ve->offset = element->offset;
        ve->components = element->components;
        ve->divisor = element->divisor;
        ve->type = toGLType[element->type];
        ve->normalize = element->normalize;
        ve->shouldConvertToFloat = element->normalize || ve->type != GL_UNSIGNED_INT;
        vertexFormat->vertexSize[stream] += GetTypeSize(element->type);
    }

    int handle = vertexFormatList.FindNull();
    if (handle == -1) {
        handle = vertexFormatList.Append(vertexFormat);
    } else {
        vertexFormatList[handle] = vertexFormat;
    }

    return (Handle)handle;
}

void OpenGLRHI::DestroyVertexFormat(Handle vertexFormatHandle) {
    if (currentContext->state->vertexFormatHandle == vertexFormatHandle) {
        SetVertexFormat(NullVertexFormat);
    }

    delete vertexFormatList[vertexFormatHandle];
    vertexFormatList[vertexFormatHandle] = nullptr;
}

int OpenGLRHI::GetTypeSize(const VertexElement::Type type) const {
    return GLTypeSize[type];
}

// ------------------------------   -------------------------
// Conventional Attribute Binding   Generic Attribute Binding
// ------------------------------   -------------------------
// vertex.position                  vertex.attrib[0]
// vertex.weight                    vertex.attrib[1]
// vertex.weight[0]                 vertex.attrib[1]
// vertex.normal                    vertex.attrib[2]
// vertex.color                     vertex.attrib[3]
// vertex.color.primary             vertex.attrib[3]
// vertex.color.secondary           vertex.attrib[4]
// vertex.fogcoord                  vertex.attrib[5]
// vertex.texcoord                  vertex.attrib[8]
// vertex.texcoord[0]               vertex.attrib[8]
// vertex.texcoord[1]               vertex.attrib[9]
// vertex.texcoord[2]               vertex.attrib[10]
// vertex.texcoord[3]               vertex.attrib[11]
// vertex.texcoord[4]               vertex.attrib[12]
// vertex.texcoord[5]               vertex.attrib[13]
// vertex.texcoord[6]               vertex.attrib[14]
// vertex.texcoord[7]               vertex.attrib[15]
// vertex.texcoord[n]               vertex.attrib[8+n]

void OpenGLRHI::SetVertexFormat(Handle vertexFormatHandle) {
    if (currentContext->state->vertexFormatHandle == vertexFormatHandle) {
        return;
    }

    const GLVertexFormat *cur = vertexFormatList[currentContext->state->vertexFormatHandle];
    const GLVertexFormat *sel = vertexFormatList[vertexFormatHandle];

#if 1
    for (int i = 0; i < VertexElement::MaxUsages; i++) {
        if (sel->vertexElements[i].components ^ cur->vertexElements[i].components) {
            if (sel->vertexElements[i].components > 0) {
                gglEnableVertexAttribArray(i);
            } else {
                gglDisableVertexAttribArray(i);
            }
        } /*else {
            GLint enabled;
            gglGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            assert((sel->vertexElements[i].components > 0 ? 0 : 1) ^ enabled);
        }*/
    }
#else
    if (sel->vertexElements[POSITION].components ^ cur->vertexElements[POSITION].components) {
        if (sel->vertexElements[POSITION].components > 0) {
            gglEnableClientState(GL_VERTEX_ARRAY);
        } else {
            gglDisableClientState(GL_VERTEX_ARRAY);
        }
    }

    if (sel->vertexElements[NORMAL].components ^ cur->vertexElements[NORMAL].components) {
        if (sel->vertexElements[NORMAL].components > 0) {
            gglEnableClientState(GL_NORMAL_ARRAY);
        } else {
            gglDisableClientState(GL_NORMAL_ARRAY);
        }
    }

    if (sel->vertexElements[COLOR].components ^ cur->vertexElements[COLOR].components) {
        if (sel->vertexElements[COLOR].components > 0) {
            gglEnableClientState(GL_COLOR_ARRAY);
        } else {
            gglDisableClientState(GL_COLOR_ARRAY);
        }
    }

    if (sel->vertexElements[SECONDARY_COLOR].components ^ cur->vertexElements[SECONDARY_COLOR].components) {
        if (sel->vertexElements[SECONDARY_COLOR].components > 0) {
            gglEnableClientState(GL_SECONDARY_COLOR_ARRAY_EXT);
        } else {
            gglDisableClientState(GL_SECONDARY_COLOR_ARRAY_EXT);
        }
    }

    for (int i = TEXCOORD0; i <= TEXCOORD7; i++) {
        if (sel->vertexElements[i].components ^ cur->vertexElements[i].components) {
            gglClientActiveTextureARB(GL_TEXTURE0 + i - TEXCOORD0);
            if (sel->vertexElements[i].components > 0) {
                gglEnableClientState(GL_TEXTURE_COORD_ARRAY);
            } else {
                gglDisableClientState(GL_TEXTURE_COORD_ARRAY);
            }
        }
    }

    for (int i = WEIGHT_INDEX0; i <= WEIGHT_VALUE1; i++) {
        if (sel->vertexElements[i].components ^ cur->vertexElements[i].components) {
            if (sel->vertexElements[i].components > 0) {
                gglEnableVertexAttribArrayARB(i - WEIGHT_INDEX0);
            } else {
                gglDisableVertexAttribArrayARB(i - WEIGHT_INDEX0);
            }
        }
    }
#endif

    currentContext->state->vertexFormatHandle = vertexFormatHandle;
}

void OpenGLRHI::SetStreamSource(int stream, Handle vertexBufferHandle, int base, int stride) {
    const GLVertexFormat *vertexFormat = vertexFormatList[currentContext->state->vertexFormatHandle];
    const GLVertexElementInternal *ve;
    GLVertexAttrib *currentVertexAttrib;

    BindBuffer(VertexBuffer, vertexBufferHandle);

#if 1
    for (int i = 0; i < VertexElement::MaxUsages; i++) {
        currentVertexAttrib = &currentContext->state->vertexAttribs[i];
        ve = &vertexFormat->vertexElements[i];

        if (ve->stream == stream && ve->components > 0) {
            if (currentVertexAttrib->vertexBufferHandle != vertexBufferHandle ||
                currentVertexAttrib->components != ve->components ||
                currentVertexAttrib->type != ve->type ||
                currentVertexAttrib->normalize != ve->normalize ||
                currentVertexAttrib->stride != stride ||
                currentVertexAttrib->ptr != BUFFER_OFFSET(base + ve->offset) ||
                currentVertexAttrib->divisor != ve->divisor) {
                if (!ve->shouldConvertToFloat) {
                    gglVertexAttribIPointer(i, ve->components, ve->type, stride, BUFFER_OFFSET(base + ve->offset));
                } else {
                    // if normalized is set to GL_TRUE, it indicates that values stored in an integer format are to be mapped to the range[-1, 1]
                    // (for signed values) or [0, 1](for unsigned values) when they are accessed and converted to floating point. Otherwise, 
                    // values will be converted to floats directly without normalization.
                    gglVertexAttribPointer(i, ve->components, ve->type, ve->normalize, stride, BUFFER_OFFSET(base + ve->offset));
                }

                OpenGL::VertexAttribDivisor(i, ve->divisor);

                currentVertexAttrib->vertexBufferHandle = vertexBufferHandle;
                currentVertexAttrib->components = ve->components;
                currentVertexAttrib->type = ve->type;
                currentVertexAttrib->normalize = ve->normalize;
                currentVertexAttrib->stride = stride;
                currentVertexAttrib->ptr = BUFFER_OFFSET(base + ve->offset);
                currentVertexAttrib->divisor = ve->divisor;
            }
        }
    }
#else
    ve = &vertexFormat->vertexElements[POSITION];
    if (ve->stream == stream && ve->components > 0) {
        gglVertexPointer(ve->components, ve->type, stride, BUFFER_OFFSET(base + ve->offset));
    }

    ve = &vertexFormat->vertexElements[NORMAL];
    if (ve->stream == stream && ve->components > 0) {
        gglNormalPointer(ve->type, stride, BUFFER_OFFSET(base + ve->offset));
    }

    ve = &vertexFormat->vertexElements[COLOR];
    if (ve->stream == stream && ve->components > 0) {
        gglColorPointer(ve->components, ve->type, stride, BUFFER_OFFSET(base + ve->offset));
    }

    ve = &vertexFormat->vertexElements[SECONDARY_COLOR];
    if (ve->stream == stream && ve->components > 0) {
        gglSecondaryColorPointerEXT(ve->components, ve->type, stride, BUFFER_OFFSET(base + ve->offset));
    }

    for (int i = 0; i < MaxTCU; i++) {
        ve = &vertexFormat->vertexElements[TEXCOORD0 + i];
        if (ve->stream == stream && ve->components > 0) {
            gglClientActiveTextureARB(GL_TEXTURE0 + i);
            gglTexCoordPointer(ve->components, ve->type, stride, BUFFER_OFFSET(base + ve->offset));
        }
    }

    for (int i = WEIGHT_INDEX0; i <= WEIGHT_VALUE1; i++) {
        ve = &vertexFormat->vertexElements[i];
        if (ve->stream == stream && ve->components > 0) {
            gglVertexAttribPointerARB(i, ve->components, ve->type, ve->normalize, stride, BUFFER_OFFSET(base + ve->offset));
        }
    }
#endif
}

BE_NAMESPACE_END
