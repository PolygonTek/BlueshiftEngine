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

RHI::Handle OpenGLRHI::CreateQuery() {
    GLuint id;
    gglGenQueries(1, &id);

    GLQuery *query = new GLQuery;
    query->id = id;

    int handle = queryList.FindNull();
    if (handle == -1) {
        handle = queryList.Append(query);
    } else {
        queryList[handle] = query;
    }

    return (Handle)handle;
}

void OpenGLRHI::DestroyQuery(Handle queryHandle) {
    GLQuery *query = queryList[queryHandle];
    gglDeleteQueries(1, &query->id);

    delete query;
    queryList[queryHandle] = nullptr;
}

void OpenGLRHI::BeginQuery(Handle queryHandle) {
    GLQuery *query = queryList[queryHandle];
    gglBeginQuery(GL_ANY_SAMPLES_PASSED, query->id);
}

void OpenGLRHI::EndQuery() {
    gglEndQuery(GL_ANY_SAMPLES_PASSED);
}

bool OpenGLRHI::QueryResultAvailable(Handle queryHandle) const {
    const GLQuery *query = queryList[queryHandle];
    GLuint available;
    gglGetQueryObjectuiv(query->id, GL_QUERY_RESULT_AVAILABLE, &available);
    return available ? true : false;
}

unsigned int OpenGLRHI::QueryResult(Handle queryHandle) const {
    const GLQuery *query = queryList[queryHandle];
    GLuint samples;
    gglGetQueryObjectuiv(query->id, GL_QUERY_RESULT, &samples);
    return samples;
}

BE_NAMESPACE_END
