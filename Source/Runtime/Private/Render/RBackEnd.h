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
-------------------------------------------------------------------------------

    RBSurf

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

struct viewEntity_t;

class RBSurf {
public:
    enum FlushType {
        BadFlush,
        SelectionFlush,
        BackgroundFlush,
        DepthFlush,
        ShadowFlush,
        OccluderFlush,
        AmbientFlush,
        LitFlush,
        UnlitFlush,
        VelocityFlush,
        FinalFlush,
        TriFlush,
        GuiFlush,
        MaxFlushTypes
    };

    void                Init();
    void                Shutdown();

    void                Begin(int flushType, const Material *material, const float *materialRegisters, const viewEntity_t *surfSpace, const viewLight_t *surfLight);
    void                DrawSubMesh(SubMesh *subMesh);
    void                Flush();

    void                EndFrame();

private:                
    void                DrawDynamicSubMesh(SubMesh *subMesh);
    void                DrawStaticSubMesh(SubMesh *subMesh);

    void                Flush_SelectionPass();
    void                Flush_BackgroundPass();
    void                Flush_DepthPass();
    void                Flush_AmbientPass();
    void                Flush_ShadowDepthPass();
    void                Flush_LitPass();
    void                Flush_UnlitPass();
    void                Flush_FinalPass();
    void                Flush_TrisPass();
    void                Flush_VelocityMapPass();
    void                Flush_GuiPass();

    void                DrawDebugWireframe(int mode, const Color4 &rgba) const;

    void                SetSubMeshVertexFormat(const SubMesh *mesh, int vertexFormatIndex) const;

    void                SetShaderProperties(const Shader *shader, const StrHashMap<Shader::Property> &shaderProperties) const;
    const Texture *     TextureFromShaderProperties(const Material::ShaderPass *mtrlPass, const Str &textureName) const;
    void                SetMatrixConstants(const Shader *shader) const;
    void                SetVertexColorConstants(const Shader *shader, const Material::VertexColorMode &vertexColor) const;
    void                SetSkinningConstants(const Shader *shader, const SkinningJointCache *cache) const;

    void                SetupLightingShader(const Material::ShaderPass *mtrlPass, const Shader *shader, bool useShadowMap) const;

    void                RenderColor(const Color4 &color) const;
    void                RenderSelection(const Material::ShaderPass *mtrlPass, const Vec3 &vec3_id) const;
    void                RenderDepth(const Material::ShaderPass *mtrlPass) const;
    void                RenderVelocity(const Material::ShaderPass *mtrlPass) const;
    void                RenderBase(const Material::ShaderPass *mtrlPass, float ambientScale) const;
    void                RenderAmbient(const Material::ShaderPass *mtrlPass, float ambientScale) const;
    void                RenderAmbientLit(const Material::ShaderPass *mtrlPass, float ambientScale) const;
    void                RenderAmbient_DirectLit(const Material::ShaderPass *mtrlPass, float ambientScale) const;
    void                RenderAmbientLit_DirectLit(const Material::ShaderPass *mtrlPass, float ambientScale) const;
    void                RenderGeneric(const Material::ShaderPass *mtrlPass) const;

    void                RenderLightInteraction(const Material::ShaderPass *mtrlPass) const;
    void                RenderFogLightInteraction(const Material::ShaderPass *mtrlPass) const;
    void                RenderBlendLightInteraction(const Material::ShaderPass *mtrlPass) const;
    void                RenderGui(const Material::ShaderPass *mtrlPass) const;

    void                DrawPrimitives() const;

    int                 flushType;
    Material *          material;
    const float *       materialRegisters;
    SubMesh *           subMesh;

    const viewEntity_t *surfSpace;
    const viewLight_t * surfLight;

    RHI::Handle         vbHandle;
    RHI::Handle         ibHandle;

    int                 startIndex;
    int                 numVerts;
    int                 numIndexes;
    int                 numInstances;
};

/*
-------------------------------------------------------------------------------

    BackEnd

-------------------------------------------------------------------------------
*/

struct LightQuery {
    const viewLight_t * light;
    RHI::Handle         queryHandle;
    unsigned int        resultSamples;
    int                 frameCount;
};

// HW skinning 용 joint cache
struct SkinningJointCache {
    int                 numJoints;              // motion blur 를 사용하면 원래 model joints 의 2배를 사용한다
    Mat3x4 *            skinningJoints;         // animation 결과 matrix(3x4) 를 담는다.
    int                 jointIndexOffsetCurr;   // motion blur 용 현재 프레임 joint index offset
    int                 jointIndexOffsetPrev;   // motion blur 용 이전 프레임 joint index offset
    BufferCache         bufferCache;            // VTF skinning 일 때 사용
    int                 viewFrameCount;         // 현재 프레임에 계산을 마쳤음을 표시하기 위한 marking number
};

struct BackEnd {
    enum PreDefinedStencilState {
        VolumeIntersectionZPass,
        VolumeIntersectionZFail,
        VolumeIntersectionInsideZFail,
        VolumeIntersectionTest,
        MaxPredefinedStencilStates
    };

    bool                initialized;
    RHI::Handle         stencilStates[MaxPredefinedStencilStates];
    //LightQuery        lightQueries[MAX_LIGHTS];

    float               time;

    RenderContext *     ctx;

    RBSurf              rbsurf;
    int                 numDrawSurfs;
    DrawSurf **         drawSurfs;
    viewEntity_t *      viewEntities;
    viewLight_t *       viewLights;
    viewLight_t *       primaryLight;
    view_t *            view;

    Rect                renderRect;
    Rect                screenRect;
    Vec2                upscaleFactor;
    double              depthMin;
    double              depthMax;
    Mat4                modelViewMatrix;
    Mat4                projMatrix;
    Mat4                modelViewProjMatrix;
    Mat4                viewMatrixPrev;

    Mat4                shadowProjectionMatrix;
    Mat4                shadowViewProjectionScaleBiasMatrix[8];
    Vec2                shadowProjectionDepth;
    float               shadowMapFilterSize[8];
    float               shadowMapOffsetFactor;
    float               shadowMapOffsetUnits;

    float               csmDistances[9];
    float               csmUpdateRatio[8];
    float               csmUpdate[8];

    Texture *           envCubeTexture;
    Texture *           integrationLUTTexture;
    Texture *           irradianceEnvCubeTexture;
    Texture *           prefilteredEnvCubeTexture;

    Texture *           homCullingOutputTexture;
    RenderTarget *      homCullingOutputRT;
};

void    RB_Init();
void    RB_Shutdown();

void    RB_Execute(const void *data);

void    RB_SetupLight(viewLight_t *viewLight);

void    RB_BackgroundPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_SelectionPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_OccluderPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_DepthPrePass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_UnlitPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_VelocityMapPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_FinalPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_DrawTris(int numDrawSurfs, DrawSurf **drawSurfs, bool forceToDraw);
void    RB_DebugPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_GuiPass(int numDrawSurfs, DrawSurf **drawSurfs);

void    RB_ShadowPass(const viewLight_t *viewLight);
void    RB_ForwardBasePass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_ForwardAdditivePass(viewLight_t *viewLights);

void    RB_PostProcessDepth();
void    RB_PostProcess();

void    RB_DrawRect(float x, float y, float x2, float y2, float s, float t, float s2, float t2);
void    RB_DrawClipRect(float s, float t, float s2, float t2);
void    RB_DrawRectSlice(float x, float y, float x2, float y2, float s, float t, float s2, float t2, float slice);
void    RB_DrawScreenRect(float x, float y, float w, float h, float s, float t, float s2, float t2);
void    RB_DrawScreenRectSlice(float x, float y, float w, float h, float s, float t, float s2, float t2, float slice);
void    RB_DrawCircle(const Vec3 &origin, const Vec3 &left, const Vec3 &up, const float radius);
void    RB_DrawAABB(const AABB &aabb);
void    RB_DrawOBB(const OBB &obb);
void    RB_DrawFrustum(const Frustum &frustum);
void    RB_DrawSphere(const Sphere &sphere, int lats, int longs);
void    RB_DrawLightVolume(const SceneLight *light);

void    RB_ClearDebugPrimitives(int time);
Vec3 *  RB_ReserveDebugPrimsVerts(int prims, int numVerts, const Color4 &color, const float lineWidth, const bool twoSided, const bool depthTest, const int lifeTime);

void    RB_ClearDebugText(int time);
void    RB_AddDebugText(const char *text, const Vec3 &origin, const Mat3 &viewAxis, float scale, float lineWidth, const Color4 &color, const int align, const int lifeTime, const bool depthTest);

extern BackEnd backEnd;

BE_NAMESPACE_END
