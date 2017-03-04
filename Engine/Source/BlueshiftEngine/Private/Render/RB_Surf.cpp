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
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

#define MAX_DYNAMIC_VERTS	(VCACHE_SIZE / sizeof(VertexLightingGeneric))

void RBSurf::Init() {
    startIndex = -1;

    vbHandle = Renderer::NullBuffer;
    ibHandle = Renderer::NullBuffer;

    numVerts = 0;
    numIndexes = 0;
    numInstances = 0;

    material = nullptr;
    subMesh = nullptr;
    surfEntity = nullptr;
}

void RBSurf::Shutdown() {
}

void RBSurf::EndFrame() {
    startIndex = -1;
}

void RBSurf::Begin(int flushType, const Material *material, const float *materialRegisters, const viewEntity_t *surfEntity, const viewLight_t *surfLight) {
    this->flushType         = flushType;
    this->material          = const_cast<Material *>(material);
    this->materialRegisters = materialRegisters;
    this->surfEntity        = surfEntity;
    this->surfLight         = surfLight;
}

void RBSurf::DrawSubMesh(SubMesh *subMesh, GuiSubMesh *guiSubMesh) {
    if (guiSubMesh) {
        DrawGuiSubMesh(guiSubMesh);
    } else {
        if (surfEntity->def->parms.joints && !subMesh->useGpuSkinning) {
            // CPU skinning mesh 일 경우 
            DrawDynamicSubMesh(subMesh);
        } else {
            DrawStaticSubMesh(subMesh);
        }
    }
}

void RBSurf::DrawStaticSubMesh(SubMesh *subMesh) {
    // 버퍼에 남아있는 dynamic mesh 를 그린다
    if (this->numIndexes) {
        Flush();
    }
    
    this->startIndex = 0;
    
    this->vbHandle = subMesh->ambientCache->buffer;
    this->ibHandle = subMesh->indexCache->buffer;
    
    this->numVerts = subMesh->numVerts;
    this->numIndexes = subMesh->numIndexes;
    
    this->subMesh = subMesh;
    
    // draw static buffer immediately!
    Flush();
}

void RBSurf::DrawDynamicSubMesh(SubMesh *subMesh) {
    if (startIndex < 0) {
        // startIndex 는 Flush 후에 -1 로 세팅된다
        startIndex = subMesh->indexCache->offset / sizeof(TriIndex);
    } else {
        // indexCache 가 순차적으로 연결되지 않는다면 한꺼번에 그릴 수 없으므로 Flush
        if (startIndex + numIndexes != subMesh->indexCache->offset / sizeof(TriIndex)) {
            Flush();
        }
    }

    this->vbHandle = subMesh->ambientCache->buffer;
    this->ibHandle = subMesh->indexCache->buffer;

    this->numVerts += subMesh->numVerts;
    this->numIndexes += subMesh->numIndexes;
    
    this->subMesh = subMesh;
}

void RBSurf::DrawGuiSubMesh(GuiSubMesh *guiSubMesh) {
    if (startIndex < 0) {
        // startIndex 는 Flush 후에 -1 로 세팅된다
        startIndex = guiSubMesh->indexCache->offset / sizeof(TriIndex);
    } else {
        // indexCache 가 순차적으로 연결되지 않는다면 한꺼번에 그릴 수 없으므로 Flush
        if (startIndex + numIndexes != guiSubMesh->indexCache->offset / sizeof(TriIndex)) {
            Flush();
        }
    }

    this->vbHandle = guiSubMesh->vertexCache->buffer;
    this->ibHandle = guiSubMesh->indexCache->buffer;

    this->numVerts += guiSubMesh->numVerts;
    this->numIndexes += guiSubMesh->numIndexes;

    this->subMesh = nullptr;
}

void RBSurf::SetSubMeshVertexFormat(const SubMesh *subMesh, int vertexFormatIndex) const {
    if (subMesh) {
        if (subMesh->useGpuSkinning) {
            glr.SetVertexFormat(vertexFormats[vertexFormatIndex + subMesh->gpuSkinningVersionIndex + 1].vertexFormatHandle);
            glr.SetStreamSource(0, vbHandle, 0, sizeof(VertexLightingGeneric));
            glr.SetStreamSource(1, vbHandle, sizeof(VertexLightingGeneric) * numVerts, subMesh->VertexWeightSize());
        } else {
            glr.SetVertexFormat(vertexFormats[vertexFormatIndex].vertexFormatHandle);
            glr.SetStreamSource(0, vbHandle, 0, sizeof(VertexLightingGeneric));
        }
    } else {
        // HACK: subMesh is nullptr for guiSubMesh
        glr.SetVertexFormat(vertexFormats[VertexFormat::PicXyzStColor].vertexFormatHandle);
        glr.SetStreamSource(0, vbHandle, 0, sizeof(VertexNoLit));
    }
}

void RBSurf::Flush() {
    if (!numIndexes) {
        return;
    }

    bool polygonOffset = false;

    if (flushType != ShadowFlush && material->flags & Material::PolygonOffset) {
        glr.SetDepthBias(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat());
        polygonOffset = true;
    }

    switch (flushType) {
    case SelectionFlush:
        Flush_SelectionPass();
        break;
    case DepthFlush:
    case OccluderFlush:
        Flush_DepthPass(); 
        break;
    case ShadowFlush:
        Flush_ShadowDepthPass(); 
        break;
    case AmbientFlush:
        Flush_AmbientPass(); 
        break;
    case LitFlush:
        Flush_LitPass();
        break;
    case BlendFlush:
        Flush_BlendPass(); 
        break;
    case VelocityFlush:
        Flush_VelocityMapPass(); 
        break;
    case FinalFlush:
        Flush_FinalPass(); 
        break;
    case TriFlush:
        Flush_TrisPass();
        break;
    case GuiFlush:
        Flush_GuiPass();
        break;
    }

    if (polygonOffset) {
        glr.SetDepthBias(0, 0);
    }

    startIndex = -1;
    
    //vbHandle = Renderer::NullBuffer;
    //ibHandle = Renderer::NullBuffer;

    numVerts = 0;
    numIndexes = 0;
    numInstances = 0;
}

void RBSurf::Flush_SelectionPass() {
    // Convert entity ID to Vec3
    int id = surfEntity->def->index;
    int b = Max(id / 65536, 0);
    int g = Max((id - b * 65536) / 256, 0);
    int r = Max(id - b * 65536 - g * 256, 0);
    Vec3 vec3_id = Vec3((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f);

    const Material::Pass *mtrlPass = material->GetPass();

    glr.SetCullFace(material->cullType);

    glr.BindBuffer(Renderer::VertexBuffer, vbHandle);

    int vertexFormatIndex = mtrlPass->stateBits & Renderer::MaskAF ? VertexFormat::GenericXyzSt : VertexFormat::GenericXyz;
    SetSubMeshVertexFormat(subMesh, vertexFormatIndex);
        
    if (mtrlPass->stateBits & Renderer::MaskAF) {
        glr.SetAlphaRef(mtrlPass->alphaRef);
    }

    int stateBits = mtrlPass->stateBits | Renderer::DepthWrite | Renderer::ColorWrite;
    stateBits &= ~Renderer::MaskBF;

    if (backEnd.view->def->parms.flags & SceneView::WireFrameMode) {
        stateBits |= Renderer::PM_Wireframe;

        glr.SetLineWidth(8);
    }

    glr.SetStateBits(stateBits);

    RenderSelection(mtrlPass, vec3_id);

    if (backEnd.view->def->parms.flags & SceneView::WireFrameMode) {
        glr.SetLineWidth(1);
    }
}

void RBSurf::Flush_DepthPass() {
    const Material::Pass *mtrlPass = material->GetPass();

    if (!(mtrlPass->stateBits & Renderer::DepthWrite) || mtrlPass->stateBits & Renderer::MaskBF) {
        return;
    }

    glr.SetCullFace(material->cullType);

    glr.BindBuffer(Renderer::VertexBuffer, vbHandle);

    int vertexFormatIndex = (mtrlPass->stateBits & Renderer::MaskAF) ? VertexFormat::GenericXyzSt : VertexFormat::GenericXyz;
    SetSubMeshVertexFormat(subMesh, vertexFormatIndex);

    if (mtrlPass->stateBits & Renderer::MaskAF) {
        glr.SetAlphaRef(mtrlPass->alphaRef);
    }

    glr.SetStateBits(mtrlPass->stateBits & (Renderer::DepthWrite | Renderer::MaskDF | Renderer::MaskAF));

    RenderDepth(mtrlPass);
}

void RBSurf::Flush_ShadowDepthPass() {
    const Material::Pass *mtrlPass = material->GetPass();

    if (!(mtrlPass->stateBits & Renderer::DepthWrite) || mtrlPass->stateBits & Renderer::MaskBF) {
        return;
    }

    glr.SetCullFace(material->cullType);
    
    glr.BindBuffer(Renderer::VertexBuffer, vbHandle);

    int vertexFormatIndex = mtrlPass->stateBits & Renderer::MaskAF ? VertexFormat::GenericXyzSt : VertexFormat::GenericXyz;
    SetSubMeshVertexFormat(subMesh, vertexFormatIndex);

    if (mtrlPass->stateBits & Renderer::MaskAF) {
        glr.SetAlphaRef(mtrlPass->alphaRef);
    }

    glr.SetStateBits(mtrlPass->stateBits & (Renderer::DepthWrite | Renderer::MaskDF | Renderer::MaskAF));

    RenderDepth(mtrlPass);
}

void RBSurf::Flush_AmbientPass() {
    const Material::Pass *mtrlPass = material->GetPass();

    if (mtrlPass->stateBits & Renderer::MaskBF) {
        return;
    }

    glr.SetCullFace(material->cullType);

    glr.BindBuffer(Renderer::VertexBuffer, vbHandle);

    int vertexFormatIndex;
    if (r_ambientLit.GetBool()) {
        vertexFormatIndex = mtrlPass->vertexColorMode != Material::IgnoreVertexColor ? VertexFormat::GenericLitColor : VertexFormat::GenericLit;
    } else {
        vertexFormatIndex = mtrlPass->vertexColorMode != Material::IgnoreVertexColor ? VertexFormat::GenericXyzStColor : VertexFormat::GenericXyzSt;
    }

    SetSubMeshVertexFormat(subMesh, vertexFormatIndex);
    
    int stateBits = mtrlPass->stateBits;

    if (r_useDepthPrePass.GetBool()) {
        stateBits &= ~(Renderer::MaskDF | Renderer::DepthWrite);
        stateBits |= Renderer::DF_Equal;
    } else {
        if (mtrlPass->stateBits & Renderer::MaskAF) {
            glr.SetAlphaRef(mtrlPass->alphaRef);
        }
    }

    glr.SetStateBits(stateBits);

    RenderAmbient(mtrlPass, r_ambientScale.GetFloat());
}

void RBSurf::Flush_LitPass() {
    const Material::Pass *mtrlPass = material->GetPass();

    if (!material->IsLitSurface()) {
        return;
    }

    glr.SetCullFace(material->cullType);
        
    glr.BindBuffer(Renderer::VertexBuffer, vbHandle);

    int vertexFormatIndex = mtrlPass->vertexColorMode != Material::IgnoreVertexColor ? VertexFormat::GenericLitColor : VertexFormat::GenericLit;
    SetSubMeshVertexFormat(subMesh, vertexFormatIndex);

    if (mtrlPass->stateBits & Renderer::MaskAF) {
        glr.SetAlphaRef(mtrlPass->alphaRef);
    }

    int stateBits = mtrlPass->stateBits;
    stateBits &= ~(Renderer::MaskDF | Renderer::DepthWrite);

    const Material *lightMaterial = surfLight->def->parms.material;
    int lightSort = lightMaterial->GetLightSort();
    switch (lightSort) {
    case Material::FogLightSort:
        glr.SetStateBits(stateBits | (Renderer::DF_Equal | Renderer::BS_SrcAlpha | Renderer::BD_OneMinusSrcAlpha));
        RenderFogLightInteraction(mtrlPass);
    case Material::BlendLightSort:
        glr.SetStateBits(stateBits | (Renderer::DF_Equal | Renderer::BS_SrcAlpha | Renderer::BD_OneMinusSrcAlpha));
        RenderBlendLightInteraction(mtrlPass);
        break;
    case Material::NormalLightSort:
        stateBits |= (Renderer::BS_One | Renderer::BD_One);
        stateBits |= material->sort == Material::AdditiveLightingSort ? Renderer::DF_LEqual : Renderer::DF_Equal;
        glr.SetStateBits(stateBits);
        RenderLightInteraction(mtrlPass);
        break;
    }
}

void RBSurf::Flush_BlendPass() {
    const Material::Pass *mtrlPass = material->GetPass();

    if (!(mtrlPass->stateBits & Renderer::MaskBF)) {
        return;
    }

    glr.SetCullFace(material->cullType);

    glr.BindBuffer(Renderer::VertexBuffer, vbHandle);
            
    SetSubMeshVertexFormat(subMesh, VertexFormat::GenericXyzStColor);

    if (mtrlPass->stateBits & Renderer::MaskAF) {
        glr.SetAlphaRef(mtrlPass->alphaRef);
    }

    glr.SetStateBits(mtrlPass->stateBits);

    RenderGeneric(mtrlPass);
}

void RBSurf::Flush_FinalPass() {
    const Material::Pass *mtrlPass = material->GetPass();

    glr.SetCullFace(material->cullType);

    glr.BindBuffer(Renderer::VertexBuffer, vbHandle);

    SetSubMeshVertexFormat(subMesh, VertexFormat::GenericLit);
        
    if (mtrlPass->stateBits & Renderer::MaskAF) {
        glr.SetAlphaRef(mtrlPass->alphaRef);
    }

    glr.SetStateBits(mtrlPass->stateBits);
                
    RenderGeneric(mtrlPass);
}

void RBSurf::Flush_TrisPass() {
    int wireframeMode;
    if (r_showWireframe.GetInteger() > 0) {
        wireframeMode = r_showWireframe.GetInteger();
    } else {
        wireframeMode = surfEntity->def->parms.wireframeMode;
    }

    DrawDebugWireframe(wireframeMode, surfEntity->def->parms.wireframeColor);	
}

void RBSurf::Flush_VelocityMapPass() {
    const Material::Pass *mtrlPass = material->GetPass();

    glr.SetCullFace(material->cullType);

    glr.BindBuffer(Renderer::VertexBuffer, vbHandle);

    SetSubMeshVertexFormat(subMesh, VertexFormat::GenericXyzNormal);

    if (mtrlPass->stateBits & Renderer::MaskAF) {
        glr.SetAlphaRef(mtrlPass->alphaRef);
    }

    int stateBits = mtrlPass->stateBits & (Renderer::ColorWrite | Renderer::AlphaWrite | Renderer::MaskDF | Renderer::MaskAF);
    stateBits |= Renderer::DepthWrite;

    glr.SetStateBits(stateBits);

    RenderVelocity(mtrlPass);
}

void RBSurf::Flush_GuiPass() {
    const Material::Pass *mtrlPass = material->GetPass();

    glr.SetCullFace(Renderer::NoCull);

    glr.BindBuffer(Renderer::VertexBuffer, vbHandle);

    SetSubMeshVertexFormat(subMesh, VertexFormat::PicXyzStColor);

    if (mtrlPass->stateBits & Renderer::MaskAF) {
        glr.SetAlphaRef(mtrlPass->alphaRef);
    }

    glr.SetStateBits(mtrlPass->stateBits & ~Renderer::DepthWrite);

    RenderGui(mtrlPass);
}

void RBSurf::DrawDebugWireframe(int mode, const Color4 &rgba) const {
    glr.BindBuffer(Renderer::VertexBuffer, vbHandle);

    SetSubMeshVertexFormat(subMesh, VertexFormat::GenericXyz);

    int blendState = 0;
    if (rgba.a < 1.0f) {
        blendState = Renderer::BS_SrcAlpha | Renderer::BD_OneMinusSrcAlpha;
    }
    
    switch (mode) {
    case SceneEntity::ShowVisibleFront:
        glr.SetStateBits(Renderer::ColorWrite | Renderer::DF_LEqual | Renderer::PM_Wireframe | blendState);	
        glr.SetCullFace(material->cullType);
        glr.SetDepthBias(-0.5f, -2.0f);
        break;
    case SceneEntity::ShowAllFront:
        glr.SetStateBits(Renderer::ColorWrite | Renderer::DF_Always | Renderer::PM_Wireframe | blendState);	
        glr.SetCullFace(material->cullType);
        break;
    case SceneEntity::ShowAllFrontAndBack:
    default:
        glr.SetStateBits(Renderer::ColorWrite | Renderer::DF_Always | Renderer::PM_Wireframe | blendState);	
        glr.SetCullFace(Renderer::NoCull);
        break;
    }

    RenderColor(rgba);

    if (mode == 1) {
        glr.SetDepthBias(0.0f, 0.0f);
    }
}
/*
void BackEnd::DrawDebugNormals(int mode) const {
    cDrawVert *vptr;
    float drawLength;
    //	float distanceScale;
    //	Vec3 vec;
    Vec3 end;
    int i;

    glr.SetStateBits(ColorWrite | DF_LEqual | DepthWrite | PM_Wireframe);

    switch (mode) {
    case 1:
        glr.SetDepthBias(0.0f, -500.0f);		
        break;
    default:
        bglDepthRange(0.0, 0.0);
        break;
    }

    bglColor3f(1.0f, 1.0f, 0.0f);

    //	distanceScale = cMath::Tan(m_view.m_fovX * 0.5f) / 100.0f;

    bglBegin(GL_LINES);

    vptr = (cDrawVert *)m_dp.systemVb;
    for (i = 0; i < numVerts; i++, vptr++) {
        //vec = m_entity->GetModelMatrix() * vptr->xyz;
        //drawLength = vec.Distance(m_view.m_vieworg) * distanceScale;
        //if (drawLength < 1.0f)
        //drawLength = 1.0f;
        drawLength = 1.0f;

        bglVertex3fv(vptr->xyz);

        end = vptr->xyz + drawLength * vptr->normal;
        bglVertex3fv(end);		
    }

    bglEnd();

    switch (mode) {
    case 1:
        glr.SetDepthBias(0.0f, 0.0f);		
        break;
    default:
        bglDepthRange(0.0, 1.0);
        break;
    }
}

void BackEnd::DrawDebugTangents(int mode) const {
    cDrawVert *vptr;
    float drawLength;
    Vec3 tan, bitan;
    //	float distanceScale;
    //	Vec3 vec;
    Vec3 end;
    int i;

    glr.SetStateBits(ColorWrite | DF_LEqual | DepthWrite | PM_Wireframe);

    switch (mode) {
    case 1:
        glr.SetDepthBias(0.0f, -500.0f);
        break;
    default:
        bglDepthRange(0.0, 0.0);
        break;
    }

    //	distanceScale = cMath::Tan(m_view.m_fovX * 0.5f) / 100.0f;

    bglBegin(GL_LINES);

    vptr = (cDrawVert *)m_dp.systemVb;
    for (i = 0; i < numVerts; i++, vptr++) {
        //vec = m_entity->GetModelMatrix() * vptr->xyz;
        //drawLength = vec.Distance(m_view.m_vieworg) * distanceScale;
        //if (drawLength < 1.0f)
        //drawLength = 1.0f;
        drawLength = 1.0f;

        tan = vptr->tangent.ToVec3();
        bitan.Cross(tan, vptr->normal);
        bitan *= vptr->tangent.w;
        bitan.NormalizeFast();

        bglColor3f(1.0f, 0.0f, 0.0f);
        bglVertex3fv(vptr->xyz);
        end = vptr->xyz + drawLength * tan;
        bglVertex3fv(end);

        bglColor3f(0.0f, 1.0f, 0.0f);
        bglVertex3fv(vptr->xyz);
        end = vptr->xyz + drawLength * bitan;
        bglVertex3fv(end);
    }

    bglEnd();

    switch (mode) {
    case 1:
        glr.SetDepthBias(0.0f, 0.0f);		
        break;
    default:
        bglDepthRange(0.0, 1.0);
        break;
    }
}

void BackEnd::DrawDebugTangentSpace(int tangentIndex) const {
    GL_BindBuffer(BGL_VERTEX_BUFFER, vbHandle);

    bglColor4ub(255, 255, 255, 255);

    glr.SetStateBits(ColorWrite | DF_LEqual);

    glr.SetCullFace((bglCullType_t)m_material->m_cullType);
    
    g_rsd.showTangentSpaceProg->Bind();
    g_rsd.showTangentSpaceProg->SetParameter1i("tangentIndex", tangentIndex);

    GL_SelectTexture(0);
    GL_ArrayOffset(BGL_TEXTURE_COORD_ARRAY, 4, BGL_FLOAT, false, sizeof(cDrawVert), OFFSET_OF(cDrawVert, tangent));
    
    bglEnableClientState(GL_NORMAL_ARRAY);
    GL_ArrayOffset(BGL_NORMAL_ARRAY, 3, BGL_FLOAT, false, sizeof(cDrawVert), OFFSET_OF(cDrawVert, normal));

    GL_ArrayOffset(BGL_VERTEX_ARRAY, 3, BGL_FLOAT, false, sizeof(cDrawVert), 0);
    
    DrawPrimitives();	
}

void BackEnd::DrawDebugBatch(const byte *rgb) const {
    GL_BindShader(BGL_NULL_SHADER);

    GL_BindBuffer(BGL_VERTEX_BUFFER, vbHandle);

    //g_textureManager.m_whiteTexture->Bind();

    bglColor3ubv(rgb);

    glr.SetStateBits(ColorWrite | DF_Equal);
    glr.SetCullFace((bglCullType_t)m_material->m_cullType);

    bglDisableClientState(GL_TEXTURE_COORD_ARRAY);

    GL_ArrayOffset(BGL_VERTEX_ARRAY, 3, BGL_FLOAT, false, sizeof(cDrawVert), 0);

    DrawPrimitives();
}*/

/*
void BackEnd::RenderFogSurface(const volumeFog_t *fog) {
    Shader *		fogMaterial;
    Plane *		fogPlane;
    byte			fogColor[4];
    Vec2 *			st;
    Vec3			v;
    Vec3			diff;
    float			dist, dist2;
    float			c;
    int				i;

    // 포그 면이 아닌 블렌딩 쉐이더는 칼라값에서 빼주므로 여기서는 무시한다.
    if (!m_material->fog && (m_material->GetSort() >= BlendSort)) {
        return;
    }

    fogPlane = fog->visibleSidePlane;
    fogMaterial = fog->material;

    // 포그 평면과 시점자와의 거리 (포그 바깥쪽이 양의 노말)
    dist = fogPlane.Distance(m_view.m_vieworg);

    st = m_drawBuffer.textureSt[0];

    for (i = 0; i < m_drawBuffer.numVerts; ++i) {
        if (rbEntity && rbEntity->type == RT_MODEL) {
            Mat3_Mul_Vector_Add_Vector(*(Mat3 *)rbEntity->axis, m_drawBuffer.xyz[i], rbEntity->origin, v);
        } else {
            Vec3_Copy(m_drawBuffer.xyz[i], v);
        }

        // 포그 평면과 월드 버텍스와의 거리
        dist2 = fogPlane.Distance(v);

        // 월드 버텍스가 포그안이라면
        if (dist2 <= 0) {
            if (dist < 0) { // 시점자가 포그 안이라면			
                diff = v - m_view.m_vieworg;
                c = cVec3_Dot(diff, m_view.m_viewaxis[ForwardAxis]) / fogMaterial->fogDistance;
            } else { // 시점자가 포그 밖이라면
                diff = v - m_view.m_vieworg;
                c = cVec3_Dot(diff, m_view.m_viewaxis[ForwardAxis]) / fogMaterial->fogDistance;
                c *= (dist2 / (dist2 - dist));
            }

            st[i][0] = c;
        } else {
            st[i][0] = 0.f;
        }		

        st[i][1] = 1.0f;//-dist2 / fogMaterial->m_fogDistance;
    }	

    // NOTE: 이전 패스가 있다면 버텍스 배열을 지정할 필요는 없다
    //bglVertexPointer(3, GL_FLOAT, 0, m_drawBuffer.xyz);

    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GL_ArrayOffset(BGL_TEXTURE_COORD_ARRAY, 2, BGL_FLOAT, false, sizeof(cDrawVert) sizeof(Vec3)*4);

    fogColor.r = fogMaterial->m_fogColor[0] * 255;
    fogColor.g = fogMaterial->m_fogColor[1] * 255;
    fogColor.b = fogMaterial->m_fogColor[2] * 255;
    fogColor.a = 255;
    bglColor4ubv(fogColor);

    g_textureManager.m_fogTexture->Bind();

    if (m_material->fog) {
        glr.SetStateBits(ColorWrite | DF_LEqual | BS_SrcAlpha | BD_OneMinusSrcAlpha);
    } else {
        glr.SetStateBits(ColorWrite | DF_Equal | BS_SrcAlpha | BD_OneMinusSrcAlpha);
    }

    DrawPrimitives();	
}

void BackEnd::ModifyColorsForFog(const stage_t *pass) {
    rBspFog_t	*fog;
    Shader		*fogShader;
    Plane		*fogPlane;
    Vec3		v;
    Vec3		diff;
    float       dist, dist2;
    float		c;
    int         i;

    fog = g_RenderMapLocal->GetFog(fogNum);
    if (!fog || !fog->visibleSidePlane) {
        return;
    }

    fogPlane = fog->visibleSidePlane;
    fogShader = fog->shader;

    // 포그 평면과 시점자와의 거리 (포그 바깥쪽이 양의 노말)
    dist = cVec3_Dot(m_view.m_vieworg, fogPlane->normal) - fogPlane->dist;

    for (i = 0; i < m_drawBuffer.numVerts; ++i) {
        if (rbEntity && rbEntity->type == RT_MODEL) {
            Vec3_Copy(rbEntity->origin, v);
        } else {
            Vec3_Copy(m_drawBuffer.xyz[i], v);
        }

        if (dist < 0) { // 시점자가 포그 안
            Vec3_Subtract(v, m_view.m_vieworg, diff);
            c = cVec3_Dot(diff, m_view.m_viewaxis[ForwardAxis]) / fogShader->fogDistance;
            c = 1.f - max(0.f, min(c, 1.f));
        } else { // 시점자가 포그 밖
            // 포그 평면과 월드버텍스와의 거리
            dist2 = cVec3_Dot(v, fogPlane->normal) - fogPlane->dist;

            if (dist2 < 0) {
                cVec3_Subtract(v, m_view.m_vieworg, diff);
                c = cVec3_Dot(diff, m_view.m_viewaxis[ForwardAxis]) / fogShader->fogDistance;
                c *= (dist2 / (dist2 - dist));
                c = 1.f - max(0.f, min(c, 1.f));
            } else {
                c = 1.f;
            }
        }

        // 알파 값을 이용한다면 버텍스 알파에 곱한다
        if ((material->state & RM_STATE_BS_SRC_ALPHA) == RM_STATE_BS_SRC_ALPHA || 
            (material->state & RM_STATE_BS_ONE_MINUS_SRC_ALPHA) == RM_STATE_BS_ONE_MINUS_SRC_ALPHA) {
            m_drawBuffer.colors[i][3] *= c;
        } else {
            m_drawBuffer.colors[i][0] *= c;
            m_drawBuffer.colors[i][1] *= c;
            m_drawBuffer.colors[i][2] *= c;
        }
    }
}
*/

BE_NAMESPACE_END
