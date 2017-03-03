#include "Precompiled.h"
#include "Renderer/RendererGL.h"
#include "RGLInternal.h"

BE_NAMESPACE_BEGIN

Renderer::Handle RendererGL::CreateQuery() {
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

void RendererGL::DeleteQuery(Handle queryHandle) {
    GLQuery *query = queryList[queryHandle];
    gglDeleteQueries(1, &query->id);

    delete query;
    queryList[queryHandle] = nullptr;
}

void RendererGL::BeginQuery(Handle queryHandle) {
    GLQuery *query = queryList[queryHandle];
    gglBeginQuery(GL_ANY_SAMPLES_PASSED, query->id);
}

void RendererGL::EndQuery() {
    gglEndQuery(GL_ANY_SAMPLES_PASSED);
}

bool RendererGL::QueryResultAvailable(Handle queryHandle) const {
    const GLQuery *query = queryList[queryHandle];
    GLuint available;
    gglGetQueryObjectuiv(query->id, GL_QUERY_RESULT_AVAILABLE, &available);
    return available ? true : false;
}

unsigned int RendererGL::QueryResult(Handle queryHandle) const {
    const GLQuery *query = queryList[queryHandle];
    GLuint samples;
    gglGetQueryObjectuiv(query->id, GL_QUERY_RESULT, &samples);
    return samples;
}

BE_NAMESPACE_END
