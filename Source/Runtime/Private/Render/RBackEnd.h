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

    Batch

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class VisObject;

class Batch {
public:
    struct Flush {
        enum Enum {
            Bad,
            Selection,
            Background,
            Depth,
            Shadow,
            Occluder,
            Base,
            Lit,
            Unlit,
            Velocity,
            Final,
            Wire,
            Gui,
            Count
        };
    };

    void                    Init();
    void                    Shutdown();

    void                    SetCurrentLight(const VisLight *surfLight);
    void                    Begin(Flush::Enum flushType, const Material *material, const float *materialRegisters, const VisObject *surfSpace);
    void                    AddInstance(const DrawSurf *drawSurf);
    void                    DrawSubMesh(SubMesh *subMesh);

    void                    Flush();

private:
    void                    DrawDynamicSubMesh(SubMesh *subMesh);
    void                    DrawStaticSubMesh(SubMesh *subMesh);

    void                    Flush_SelectionPass();
    void                    Flush_BackgroundPass();
    void                    Flush_DepthPass();
    void                    Flush_DepthNormalPass();
    void                    Flush_BasePass();
    void                    Flush_ShadowDepthPass();
    void                    Flush_LitPass();
    void                    Flush_UnlitPass();

    void                    Flush_FinalPass();
    void                    Flush_WirePass();
    void                    Flush_VelocityMapPass();
    void                    Flush_GuiPass();

    void                    DrawDebugWireframe(int mode, const Color4 &rgba) const;

    void                    SetSubMeshVertexFormat(const SubMesh *mesh, int vertexFormatIndex) const;

    void                    SetShaderProperties(const Shader *shader, const StrHashMap<Shader::Property> &shaderProperties) const;
    const Texture *         TextureFromShaderProperties(const Material::ShaderPass *mtrlPass, const Str &textureName) const;
    void                    SetMatrixConstants(const Shader *shader) const;
    void                    SetVertexColorConstants(const Shader *shader, const Material::VertexColorMode::Enum &vertexColor) const;
    void                    SetSkinningConstants(const Shader *shader, const SkinningJointCache *cache) const;
    void                    SetEntityConstants(const Material::ShaderPass *mtrlPass, const Shader *shader) const;
    void                    SetProbeConstants(const Shader *shader) const;
    void                    SetMaterialConstants(const Material::ShaderPass *mtrlPass, const Shader *shader) const;

    void                    SetupLightingShader(const Material::ShaderPass *mtrlPass, const Shader *shader, bool useShadowMap) const;

    void                    RenderColor(const Material::ShaderPass *mtrlPass, const Color4 &color) const;
    void                    RenderSelection(const Material::ShaderPass *mtrlPass, const Vec3 &idInVec3) const;
    void                    RenderDepth(const Material::ShaderPass *mtrlPass) const;
    void                    RenderDepthNormal(const Material::ShaderPass *mtrlPass) const;
    void                    RenderVelocity(const Material::ShaderPass *mtrlPass) const;
    void                    RenderBase(const Material::ShaderPass *mtrlPass, float ambientScale) const;
    void                    RenderAmbient(const Material::ShaderPass *mtrlPass, float ambientScale) const;
    void                    RenderAmbient_DirectLit(const Material::ShaderPass *mtrlPass, float ambientScale) const;
    void                    RenderIndirectLit(const Material::ShaderPass *mtrlPass) const;
    void                    RenderIndirectLit_DirectLit(const Material::ShaderPass *mtrlPass) const;
    void                    RenderGeneric(const Material::ShaderPass *mtrlPass) const;

    void                    RenderLightInteraction(const Material::ShaderPass *mtrlPass) const;
    void                    RenderFogLightInteraction(const Material::ShaderPass *mtrlPass) const;
    void                    RenderBlendLightInteraction(const Material::ShaderPass *mtrlPass) const;
    void                    RenderGui(const Material::ShaderPass *mtrlPass) const;

    void                    DrawPrimitives() const;

    int                     flushType;
    Material *              material;
    const float *           materialRegisters;
    SubMesh *               subMesh;

    const VisObject *       surfSpace;
    const VisLight *        surfLight;

    RHI::Handle             vertexBuffer;
    RHI::Handle             indexBuffer;
    RHI::Handle             indirectBuffer;

    int                     startIndex;
    int                     numVerts;
    int                     numIndexes;

    int                     maxInstancingCount;
    int                     numInstances;

    // variables for instancing with uniform buffer
    int                     instanceStartIndex;
    int                     instanceEndIndex;
    int *                   instanceLocalIndexes;

    // variables for instancing with instanced array
    RHI::DrawElementsIndirectCommand *indirectCommands;
    int                     numIndirectCommands;
};

/*
-------------------------------------------------------------------------------

    RenderBackEnd

-------------------------------------------------------------------------------
*/

struct LightQuery {
    const VisLight *        light;
    RHI::Handle             queryHandle;
    unsigned int            resultSamples;
    int                     frameCount;
};

struct RenderBackEnd {
    struct PreDefinedStencilState {
        enum Enum {
            VolumeIntersectionZPass,
            VolumeIntersectionZFail,
            VolumeIntersectionInsideZFail,
            VolumeIntersectionTest,
            Count
        };
    };

    bool                    initialized;
    RHI::Handle             stencilStates[PreDefinedStencilState::Count];
    //LightQuery            lightQueries[MAX_LIGHTS];

    float                   time;

    RenderContext *         ctx;

    Batch                   batch;
    int                     numDrawSurfs;
    int                     numAmbientSurfs;
    DrawSurf **             drawSurfs;
    BufferCache *           instanceBufferCache;
    LinkList<VisObject> *   visObjects;
    LinkList<VisLight> *    visLights;
    VisLight *              primaryLight;
    VisCamera *             camera;

    Rect                    renderRect;
    Vec2                    upscaleFactor;
    double                  depthMin;
    double                  depthMax; 
    bool                    useDepthPrePass;

    Mat4                    projMatrix;
    Mat4                    viewMatrix;
    Mat4                    viewProjMatrix;
    Mat4                    viewMatrixPrev;

    Mat4                    modelViewMatrix;
    Mat4                    modelViewProjMatrix;

    Mat4                    shadowProjectionMatrix;
    Mat4                    shadowViewProjectionScaleBiasMatrix[8];
    Vec2                    shadowProjectionDepth;
    float                   shadowMapFilterSize[8];
    float                   shadowMapOffsetFactor;
    float                   shadowMapOffsetUnits;

    float                   csmDistances[9];
    float                   csmUpdateRatio[8];
    float                   csmUpdate[8];
    float                   csmFar[8];

    Texture *               dfgSumGgxTexture;

    Texture *               homCullingOutputTexture;
    RenderTarget *          homCullingOutputRT;
};

void    RB_Init();
void    RB_Shutdown();

void    RB_Execute(const void *data);

void    RB_SetupLight(VisLight *visLight);

void    RB_BackgroundPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_SelectionPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_OccluderPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_DepthPrePass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_BlendPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_VelocityMapPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_FinalPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_DrawTris(int numDrawSurfs, DrawSurf **drawSurfs, bool forceToDraw);
void    RB_DebugToolsPass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_GuiPass(int numDrawSurfs, DrawSurf **drawSurfs);

void    RB_ShadowPass(const VisLight *visLight);
void    RB_ForwardBasePass(int numDrawSurfs, DrawSurf **drawSurfs);
void    RB_ForwardAdditivePass(const LinkList<VisLight> *visLights);

void    RB_PostProcessDepth();
void    RB_PostProcess();

void    RB_DrawLightVolume(const RenderLight *light);

void    RB_ClearDebugPrimitives(int time);
Vec3 *  RB_ReserveDebugPrimsVerts(int prims, int numVerts, const Color4 &color, const float lineWidth, const bool twoSided, const bool depthTest, const int lifeTime);

void    RB_ClearDebugText(int time);
void    RB_AddDebugText(const char *text, const Vec3 &origin, const Mat3 &viewAxis, float scale, float lineWidth, const Color4 &color, const int align, const int lifeTime, const bool depthTest);

extern RenderBackEnd backEnd;

BE_NAMESPACE_END
