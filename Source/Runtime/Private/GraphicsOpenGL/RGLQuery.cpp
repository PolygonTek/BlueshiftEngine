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
#include "Graphics/GraphicsOpenGL.h"
#include "RGLInternal.h"

BE_NAMESPACE_BEGIN

Graphics::Handle GraphicsOpenGL::CreateQuery(QueryType::Enum queryType) {
    GLuint id;
    gglGenQueries(1, &id);

    GLQuery *query = new GLQuery;
    query->queryType = queryType;
    query->id = id;

    int handle = queryList.FindNull();
    if (handle == -1) {
        handle = queryList.Append(query);
    } else {
        queryList[handle] = query;
    }

    return (Handle)handle;
}

void GraphicsOpenGL::DestroyQuery(Handle queryHandle) {
    GLQuery *query = queryList[queryHandle];

    gglDeleteQueries(1, &query->id);

    delete query;
    queryList[queryHandle] = nullptr;
}

void GraphicsOpenGL::BeginQuery(Handle queryHandle) {
    const GLQuery *query = queryList[queryHandle];

    if (query->queryType == Graphics::QueryType::Occlusion) {
        gglBeginQuery(GL_ANY_SAMPLES_PASSED, query->id);
    }
}

void GraphicsOpenGL::EndQuery(Handle queryHandle) {
    const GLQuery *query = queryList[queryHandle];

    if (query->queryType == Graphics::QueryType::Occlusion) {
        gglEndQuery(GL_ANY_SAMPLES_PASSED);
    }
}

void GraphicsOpenGL::QueryTimestamp(Handle queryHandle) {
    const GLQuery *query = queryList[queryHandle];

    if (OpenGL::SupportsTimestampQueries() && query->queryType == Graphics::QueryType::Timestamp) {
        OpenGL::QueryTimestampCounter(query->id);
    }
}

bool GraphicsOpenGL::QueryResultAvailable(Handle queryHandle) const {
    const GLQuery *query = queryList[queryHandle];

    GLuint available;
    gglGetQueryObjectuiv(query->id, GL_QUERY_RESULT_AVAILABLE, &available);
    return available ? true : false;
}

uint64_t GraphicsOpenGL::QueryResult(Handle queryHandle) const {
    const GLQuery *query = queryList[queryHandle];

    uint64_t result;
    if (OpenGL::SupportsTimestampQueries()) {
        result = OpenGL::QueryResult64(query->id);
    } else {
        result = OpenGL::QueryResult32(query->id);
    }
    return result;
}

BE_NAMESPACE_END
