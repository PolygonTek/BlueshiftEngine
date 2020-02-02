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
#include "Core/Heap.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "RBackEnd.h"

BE_NAMESPACE_BEGIN

void Batch::Init() {
    vertexBuffer = RHI::NullBuffer;
    indexBuffer = RHI::NullBuffer;
    indirectBuffer = RHI::NullBuffer;

    startIndex = -1;

    numVerts = 0;
    numIndexes = 0;
    numInstances = 0;
    numIndirectCommands = 0;

    instanceStartIndex = -1;
    instanceEndIndex = -1;

    instanceLocalIndexes = nullptr;
    indirectCommands = nullptr;

    if (renderGlobal.instancingMethod == Mesh::InstancingMethod::InstancedArrays) {
        indirectBuffer = rhi.CreateBuffer(RHI::BufferType::DrawIndirect, RHI::BufferUsage::Stream, 0);

        maxInstancingCount = r_maxInstancingCount.GetInteger();

        if (maxInstancingCount > 0) {
            indirectCommands = (RHI::DrawElementsIndirectCommand *)Mem_Alloc16(maxInstancingCount * sizeof(indirectCommands[0]));
        }
    } else if (renderGlobal.instancingMethod == Mesh::InstancingMethod::UniformBuffer) {
        indirectBuffer = rhi.CreateBuffer(RHI::BufferType::DrawIndirect, RHI::BufferUsage::Stream, 0);

        maxInstancingCount = Min(r_maxInstancingCount.GetInteger(), rhi.HWLimit().maxUniformBlockSize / renderGlobal.instanceBufferOffsetAlignment);

        if (maxInstancingCount > 0) {
            instanceLocalIndexes = (int *)Mem_Alloc16(maxInstancingCount * sizeof(instanceLocalIndexes[0]));
        }
    } else {
        maxInstancingCount = 0;
    }

    material = nullptr;
    subMesh = nullptr;

    surfSpace = nullptr;
    surfLight = nullptr;
}

void Batch::Shutdown() {
    if (instanceLocalIndexes) {
        Mem_AlignedFree(instanceLocalIndexes);
        instanceLocalIndexes = nullptr;
    }
    if (indirectCommands) {
        Mem_AlignedFree(indirectCommands);
        indirectCommands = nullptr;
    }

    rhi.DestroyBuffer(indirectBuffer);
}

void Batch::SetCurrentLight(const VisLight *surfLight) {
    this->surfLight = surfLight;
}

void Batch::Begin(Flush::Enum flushType, const Material *material, const float *materialRegisters, const VisObject *surfSpace) {
    this->flushType = flushType;
    this->material = const_cast<Material *>(material);
    this->materialRegisters = materialRegisters;
    this->surfSpace = surfSpace;
}

void Batch::AddInstance(const DrawSurf *drawSurf) {
    if (renderGlobal.instancingMethod == Mesh::InstancingMethod::InstancedArrays) {
        if (numIndirectCommands > 0) {
            RHI::DrawElementsIndirectCommand *currentIndirectCommand = &indirectCommands[numIndirectCommands - 1];

            // Check if continuous instance index
            if (currentIndirectCommand->baseInstance + currentIndirectCommand->instanceCount == drawSurf->space->instanceIndex) {
                if (currentIndirectCommand->instanceCount < maxInstancingCount) {
                    currentIndirectCommand->instanceCount++;
                    numInstances++;
                    return;
                }

                Flush();
            } else {
                if (numIndirectCommands >= maxInstancingCount) {
                    Flush();
                }
            }
        }

        indirectCommands[numIndirectCommands].vertexCount = drawSurf->subMesh->numIndexes;
        indirectCommands[numIndirectCommands].instanceCount = 1;
        indirectCommands[numIndirectCommands].firstIndex = 0;
        indirectCommands[numIndirectCommands].baseVertex = 0;
        indirectCommands[numIndirectCommands].baseInstance = drawSurf->space->instanceIndex;
        numIndirectCommands++;
        numInstances++;
    } else {
        //assert(renderGlobal.instancingMethod == Mesh::InstancingMethod::UniformBuffer);
        if (instanceStartIndex < 0) {
            instanceStartIndex = drawSurf->space->instanceIndex;
        } else if (drawSurf->space->instanceIndex - instanceStartIndex + 1 >= maxInstancingCount) {
            Flush();

            instanceStartIndex = drawSurf->space->instanceIndex;
        }

        instanceEndIndex = drawSurf->space->instanceIndex;

        instanceLocalIndexes[numInstances] = drawSurf->space->instanceIndex - instanceStartIndex;

        numInstances++;
    }
}

void Batch::DrawSubMesh(SubMesh *subMesh) {
    if (subMesh->GetType() == Mesh::Type::Reference ||
        subMesh->GetType() == Mesh::Type::Static ||
        subMesh->GetType() == Mesh::Type::Skinned) {
        DrawStaticSubMesh(subMesh);
    } else {
        DrawDynamicSubMesh(subMesh);
    }
}

void Batch::DrawStaticSubMesh(SubMesh *subMesh) {
    if (this->subMesh && this->subMesh->refSubMesh != subMesh->refSubMesh) {
        Flush();
    }

    if (!this->subMesh || this->subMesh->refSubMesh != subMesh->refSubMesh) {
        this->subMesh = subMesh;

        vertexBuffer = subMesh->vertexCache->buffer;
        indexBuffer = subMesh->indexCache->buffer;

        numVerts = subMesh->numVerts;
        numIndexes = subMesh->numIndexes;

        startIndex = 0;
    }
}

void Batch::DrawDynamicSubMesh(SubMesh *subMesh) {
    if (startIndex < 0) {
        // startIndex is set to -1 after Flush()
        startIndex = subMesh->indexCache->offset / sizeof(TriIndex);
    } else {
        // Flush because indexCache cannot be drawn all at once unless they are connected in sequence.
        if (startIndex + numIndexes != subMesh->indexCache->offset / sizeof(TriIndex) ||
            vertexBuffer != subMesh->vertexCache->buffer ||
            indexBuffer != subMesh->indexCache->buffer) {
            Flush();

            startIndex = subMesh->indexCache->offset / sizeof(TriIndex);
        }
    }

    vertexBuffer = subMesh->vertexCache->buffer;
    indexBuffer = subMesh->indexCache->buffer;

    numVerts += subMesh->numVerts;
    numIndexes += subMesh->numIndexes;

    this->subMesh = subMesh;
}

void Batch::SetSubMeshVertexFormat(const SubMesh *subMesh, int vertexFormatIndex) const {
    // HACK!!
    // TODO: check vertex type of the subMesh instead of this
    int vertexSize = subMesh->GetType() != Mesh::Type::Dynamic ? sizeof(VertexGenericLit) : sizeof(VertexGeneric);

    if (numIndirectCommands > 0 && renderGlobal.instancingMethod == Mesh::InstancingMethod::InstancedArrays) {
        if (subMesh->useGpuSkinning) {
            rhi.SetVertexFormat(vertexFormats[vertexFormatIndex + 4 + subMesh->gpuSkinningVersionIndex + 1].vertexFormatHandle);

            rhi.SetStreamSource(0, vertexBuffer, 0, vertexSize);
            rhi.SetStreamSource(1, vertexBuffer, vertexSize * numVerts, subMesh->VertexWeightSize());
            rhi.SetStreamSource(2, backEnd.instanceBufferCache->buffer, backEnd.instanceBufferCache->offset, renderGlobal.instanceBufferOffsetAlignment);
        } else {
            rhi.SetVertexFormat(vertexFormats[vertexFormatIndex + 4].vertexFormatHandle);

            rhi.SetStreamSource(0, vertexBuffer, 0, vertexSize);
            rhi.SetStreamSource(1, backEnd.instanceBufferCache->buffer, backEnd.instanceBufferCache->offset, renderGlobal.instanceBufferOffsetAlignment);
        }
    } else {
        if (subMesh->useGpuSkinning) {
            rhi.SetVertexFormat(vertexFormats[vertexFormatIndex + subMesh->gpuSkinningVersionIndex + 1].vertexFormatHandle);

            rhi.SetStreamSource(0, vertexBuffer, 0, vertexSize);
            rhi.SetStreamSource(1, vertexBuffer, vertexSize * numVerts, subMesh->VertexWeightSize());
        } else {
            rhi.SetVertexFormat(vertexFormats[vertexFormatIndex].vertexFormatHandle);

            rhi.SetStreamSource(0, vertexBuffer, 0, vertexSize);
        }
    }
}

void Batch::Flush() {
    if (!numIndexes) {
        return;
    }

    bool polygonOffset = false;

    if (flushType != Flush::Shadow && (material->flags & Material::Flag::PolygonOffset)) {
        rhi.SetDepthBias(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat());
        polygonOffset = true;
    }

    switch (flushType) {
    case Flush::Selection:
        Flush_SelectionPass();
        break;
    case Flush::Background:
        Flush_BackgroundPass();
        break;
    case Flush::Depth:
        Flush_DepthPass();
        break;
    case Flush::Occluder:
        Flush_DepthPass();
        break;
    case Flush::Shadow:
        Flush_ShadowDepthPass();
        break;
    case Flush::Base:
        Flush_BasePass();
        break;
    case Flush::Lit:
        Flush_LitPass();
        break;
    case Flush::Unlit:
        Flush_UnlitPass();
        break;
    case Flush::Velocity:
        Flush_VelocityMapPass();
        break;
    case Flush::Final:
        Flush_FinalPass();
        break;
    case Flush::Wire:
        Flush_WirePass();
        break;
    case Flush::Gui:
        Flush_GuiPass();
        break;
    }

    if (polygonOffset) {
        rhi.SetDepthBias(0, 0);
    }

    startIndex = -1;

    //vertexBuffer = RHI::NullBuffer;
    //indexBuffer = RHI::NullBuffer;

    subMesh = nullptr;

    numVerts = 0;
    numIndexes = 0;

    numInstances = 0;
    numIndirectCommands = 0;

    instanceStartIndex = -1;
    instanceEndIndex = -1;
}

// Converts 24-bit ID to Vec3
static Vec3 MakeVec3Id(uint32_t id) {
    assert((id & 0xFF000000) == 0);
    const uint32_t b = Max<uint32_t>(id >> 16, 0);
    id -= (b << 16);
    const uint32_t g = Max<uint32_t>(id >> 8, 0);
    id -= (g << 8);
    const uint32_t r = Max<uint32_t>(id, 0);

    return Vec3((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f);
}

// Converts 32-bit ID to Vec4
static Vec4 MakeVec4Id(uint32_t id) {
    const uint32_t a = Max<uint32_t>(id >> 24, 0);
    id -= (a << 24);
    const uint32_t b = Max<uint32_t>(id >> 16, 0);
    id -= (b << 16);
    const uint32_t g = Max<uint32_t>(id >> 8, 0);
    id -= (g << 8);
    const uint32_t r = Max<uint32_t>(id, 0);

    return Vec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
}

void Batch::Flush_SelectionPass() {
    const Vec3 id = MakeVec3Id(surfSpace->def->GetIndex());

    const Material::ShaderPass *mtrlPass = material->GetPass();

    int depthTestBits = (surfSpace->def->GetState().flags & RenderObject::Flag::UseRenderingOrder) ? 0 : mtrlPass->depthTestBits;

    rhi.SetCullFace(mtrlPass->cullType);

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    int vertexFormatIndex = (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) ?
        VertexFormat::Type::GenericXyzSt : VertexFormat::Type::GenericXyz;
    SetSubMeshVertexFormat(subMesh, vertexFormatIndex);

    int stateBits = mtrlPass->stateBits | RHI::DepthWrite | RHI::ColorWrite | depthTestBits;
    stateBits &= ~RHI::MaskBF;

    if (backEnd.camera->def->GetState().flags & RenderCamera::Flag::WireFrameMode) {
        stateBits |= RHI::PM_Wireframe;

        rhi.SetLineWidth(8);
    }

    rhi.SetStateBits(stateBits);

    RenderSelection(mtrlPass, id);

    if (backEnd.camera->def->GetState().flags & RenderCamera::Flag::WireFrameMode) {
        rhi.SetLineWidth(1);
    }
}

void Batch::Flush_BackgroundPass() {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    int depthTestBits = (surfSpace->def->GetState().flags & RenderObject::Flag::UseRenderingOrder) ? 0 : mtrlPass->depthTestBits;

    rhi.SetCullFace(mtrlPass->cullType);

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    SetSubMeshVertexFormat(subMesh, VertexFormat::Type::GenericXyzSt);

    rhi.SetStateBits(mtrlPass->stateBits | depthTestBits);

    RenderGeneric(mtrlPass);
}

void Batch::Flush_DepthPass() {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    if (!(mtrlPass->stateBits & RHI::DepthWrite) || mtrlPass->stateBits & RHI::MaskBF) {
        return;
    }

    int depthTestBits = (surfSpace->def->GetState().flags & RenderObject::Flag::UseRenderingOrder) ? 0 : mtrlPass->depthTestBits;

    rhi.SetCullFace(mtrlPass->cullType);

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    int vertexFormatIndex = (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) ?
        VertexFormat::Type::GenericXyzStColor : VertexFormat::Type::GenericXyz;
    SetSubMeshVertexFormat(subMesh, vertexFormatIndex);

    rhi.SetStateBits(RHI::DepthWrite | depthTestBits);

    RenderDepth(mtrlPass);
}

void Batch::Flush_DepthNormalPass() {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    if (!(mtrlPass->stateBits & RHI::DepthWrite) || mtrlPass->stateBits & RHI::MaskBF) {
        return;
    }

    int depthTestBits = (surfSpace->def->GetState().flags & RenderObject::Flag::UseRenderingOrder) ? 0 : mtrlPass->depthTestBits;

    rhi.SetCullFace(mtrlPass->cullType);

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    SetSubMeshVertexFormat(subMesh, VertexFormat::Type::GenericXyzStNT);

    rhi.SetStateBits(RHI::DepthWrite | depthTestBits);

    RenderDepthNormal(mtrlPass);
}

void Batch::Flush_ShadowDepthPass() {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    if (!(mtrlPass->stateBits & RHI::DepthWrite) || mtrlPass->stateBits & RHI::MaskBF) {
        return;
    }

    rhi.SetCullFace(mtrlPass->cullType);

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    int vertexFormatIndex = (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) ?
        VertexFormat::Type::GenericXyzSt : VertexFormat::Type::GenericXyz;
    SetSubMeshVertexFormat(subMesh, vertexFormatIndex);

    rhi.SetStateBits(RHI::DepthWrite | RHI::DF_LEqual);

    RenderDepth(mtrlPass);
}

void Batch::Flush_BasePass() {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    rhi.SetCullFace(mtrlPass->cullType);

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    int vertexFormatIndex = mtrlPass->vertexColorMode != Material::VertexColorMode::Ignore ?
        VertexFormat::Type::GenericXyzStColorNT : VertexFormat::Type::GenericXyzStNT;

    SetSubMeshVertexFormat(subMesh, vertexFormatIndex);

    int stateBits = mtrlPass->stateBits;

    int depthTestBits = (surfSpace->def->GetState().flags & RenderObject::Flag::UseRenderingOrder) ? 0 : mtrlPass->depthTestBits;

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaBlend) {
        stateBits |= (RHI::BS_SrcAlpha | RHI::BD_OneMinusSrcAlpha);

        if (mtrlPass->transparency == Material::Transparency::TwoPassesOneSide) {
            rhi.SetStateBits((stateBits & ~(RHI::MaskBF | RHI::ColorWrite)) | RHI::DepthWrite | depthTestBits);
            RenderDepth(mtrlPass);

            rhi.SetStateBits(stateBits | RHI::DF_Equal);
            RenderBase(mtrlPass, r_ambientScale.GetFloat());
        } else if (mtrlPass->transparency == Material::Transparency::TwoPassesTwoSides) {
            rhi.SetStateBits(stateBits | depthTestBits);
            rhi.SetCullFace(RHI::CullType::Front);
            RenderBase(mtrlPass, r_ambientScale.GetFloat());

            rhi.SetCullFace(RHI::CullType::Back);
            DrawPrimitives();
        } else {
            rhi.SetStateBits(stateBits | depthTestBits);
            RenderBase(mtrlPass, r_ambientScale.GetFloat());
        }
    } else {
        if (backEnd.useDepthPrePass) {
            stateBits &= ~RHI::DepthWrite;
            stateBits |= RHI::DF_Equal;
        } else {
            stateBits |= depthTestBits;
        }

        rhi.SetStateBits(stateBits);
        RenderBase(mtrlPass, r_ambientScale.GetFloat());
    }
}

void Batch::Flush_UnlitPass() {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    SetSubMeshVertexFormat(subMesh, VertexFormat::Type::GenericXyzStColor);

    int depthTestBits = (surfSpace->def->GetState().flags & RenderObject::Flag::UseRenderingOrder) ? 0 : mtrlPass->depthTestBits;

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaBlend) {
        if (mtrlPass->transparency == Material::Transparency::TwoPassesOneSide) {
            rhi.SetStateBits((mtrlPass->stateBits & ~(RHI::MaskBF | RHI::ColorWrite)) | RHI::DepthWrite | depthTestBits);
            RenderDepth(mtrlPass);

            rhi.SetStateBits(mtrlPass->stateBits | RHI::DF_Equal);
            RenderGeneric(mtrlPass);
        } else if (mtrlPass->transparency == Material::Transparency::TwoPassesTwoSides) {
            rhi.SetStateBits(mtrlPass->stateBits | depthTestBits);
            rhi.SetCullFace(RHI::CullType::Front);
            RenderGeneric(mtrlPass);

            rhi.SetCullFace(RHI::CullType::Back);
            DrawPrimitives();
        } else {
            rhi.SetStateBits(mtrlPass->stateBits | depthTestBits);
            rhi.SetCullFace(mtrlPass->cullType);
            RenderGeneric(mtrlPass);
        }
    } else {
        rhi.SetStateBits(mtrlPass->stateBits | depthTestBits);
        rhi.SetCullFace(mtrlPass->cullType);
        RenderGeneric(mtrlPass);
    }
}

void Batch::Flush_LitPass() {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    if (!material->IsLitSurface()) {
        return;
    }

    rhi.SetCullFace(mtrlPass->cullType);

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    int vertexFormatIndex = mtrlPass->vertexColorMode != Material::VertexColorMode::Ignore ?
        VertexFormat::Type::GenericXyzStColorNT : VertexFormat::Type::GenericXyzStNT;
    SetSubMeshVertexFormat(subMesh, vertexFormatIndex);

    int stateBits = mtrlPass->stateBits;
    stateBits &= ~RHI::DepthWrite;
    stateBits &= ~RHI::MaskBF;

    int depthTestBits = (surfSpace->def->GetState().flags & RenderObject::Flag::UseRenderingOrder) ? 0 : mtrlPass->depthTestBits;

    const Material *lightMaterial = surfLight->def->GetState().material;
    int lightMaterialType = lightMaterial->GetType();
    switch (lightMaterialType) {
    case Material::Type::FogLight:
        rhi.SetStateBits(stateBits | (RHI::DF_Equal | RHI::BS_SrcAlpha | RHI::BD_OneMinusSrcAlpha));
        RenderFogLightInteraction(mtrlPass);
    case Material::Type::BlendLight:
        rhi.SetStateBits(stateBits | (RHI::DF_Equal | RHI::BS_SrcAlpha | RHI::BD_OneMinusSrcAlpha));
        RenderBlendLightInteraction(mtrlPass);
        break;
    case Material::Type::Light:
        if (mtrlPass->renderingMode == Material::RenderingMode::AlphaBlend) {
            if (mtrlPass->transparency == Material::Transparency::TwoPassesOneSide) {
                rhi.SetStateBits(stateBits | RHI::BS_SrcAlpha | RHI::BD_One | RHI::DF_Equal);
                RenderLightInteraction(mtrlPass);
            } else if (mtrlPass->transparency == Material::Transparency::TwoPassesTwoSides) {
                rhi.SetStateBits(stateBits | RHI::BS_SrcAlpha | RHI::BD_One | depthTestBits);
                rhi.SetCullFace(RHI::CullType::Front);
                RenderLightInteraction(mtrlPass);

                rhi.SetCullFace(RHI::CullType::Back);
                DrawPrimitives();
            } else {
                rhi.SetStateBits(stateBits | RHI::BS_SrcAlpha | RHI::BD_One | depthTestBits);
                RenderLightInteraction(mtrlPass);
            }
        } else {
            rhi.SetStateBits(stateBits | RHI::BS_One | RHI::BD_One | RHI::DF_Equal);
            RenderLightInteraction(mtrlPass);
        }
        break;
    }
}

void Batch::Flush_FinalPass() {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    int depthTestBits = (surfSpace->def->GetState().flags & RenderObject::Flag::UseRenderingOrder) ? 0 : mtrlPass->depthTestBits;

    rhi.SetCullFace(mtrlPass->cullType);

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    SetSubMeshVertexFormat(subMesh, VertexFormat::Type::GenericXyzStNT);

    rhi.SetStateBits(mtrlPass->stateBits | depthTestBits);

    RenderGeneric(mtrlPass);
}

void Batch::Flush_WirePass() {
    int wireframeMode;
    if (r_showWireframe.GetInteger() > 0) {
        wireframeMode = r_showWireframe.GetInteger();
    } else {
        wireframeMode = surfSpace->def->GetState().wireframeMode;
    }

    DrawDebugWireframe(wireframeMode, surfSpace->def->GetState().wireframeColor);
}

void Batch::Flush_VelocityMapPass() {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    int depthTestBits = (surfSpace->def->GetState().flags & RenderObject::Flag::UseRenderingOrder) ? 0 : mtrlPass->depthTestBits;

    rhi.SetCullFace(mtrlPass->cullType);

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    SetSubMeshVertexFormat(subMesh, VertexFormat::Type::GenericXyzNormal);

    int stateBits = mtrlPass->stateBits & (RHI::ColorWrite | RHI::AlphaWrite);
    stateBits |= RHI::DepthWrite;

    rhi.SetStateBits(stateBits | depthTestBits);

    RenderVelocity(mtrlPass);
}

void Batch::Flush_GuiPass() {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    int depthTestBits = (surfSpace->def->GetState().flags & RenderObject::Flag::UseRenderingOrder) ? 0 : mtrlPass->depthTestBits;

    rhi.SetCullFace(RHI::CullType::None);

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    SetSubMeshVertexFormat(subMesh, VertexFormat::Type::GenericXyzStColor);

    int stateBits = mtrlPass->stateBits;
    stateBits &= ~RHI::DepthWrite;

    rhi.SetStateBits(stateBits | depthTestBits);

    RenderGui(mtrlPass);
}

void Batch::DrawDebugWireframe(int mode, const Color4 &rgba) const {
    const Material::ShaderPass *mtrlPass = material->GetPass();

    rhi.BindBuffer(RHI::BufferType::Vertex, vertexBuffer);

    SetSubMeshVertexFormat(subMesh, VertexFormat::Type::GenericXyz);

    int blendState = 0;
    if (rgba.a < 1.0f) {
        blendState = RHI::BS_SrcAlpha | RHI::BD_OneMinusSrcAlpha;
    }

    if (mode == RenderObject::WireframeMode::ShowNone) {
        mode = RenderObject::WireframeMode::ShowVisibleFront;
    }

    switch (mode) {
    case RenderObject::WireframeMode::ShowVisibleFront:
        rhi.SetStateBits(RHI::ColorWrite | RHI::DF_LEqual | RHI::PM_Wireframe | blendState);
        rhi.SetCullFace(mtrlPass->cullType);
        rhi.SetDepthBias(-0.5f, -2.0f);
        break;
    case RenderObject::WireframeMode::ShowAllFront:
        rhi.SetStateBits(RHI::ColorWrite | RHI::DF_Always | RHI::PM_Wireframe | blendState);
        rhi.SetCullFace(mtrlPass->cullType);
        break;
    case RenderObject::WireframeMode::ShowAllFrontAndBack:
        rhi.SetStateBits(RHI::ColorWrite | RHI::DF_Always | RHI::PM_Wireframe | blendState);
        rhi.SetCullFace(RHI::CullType::None);
        break;
    }

    RenderColor(mtrlPass, rgba);

    if (mode == RenderObject::WireframeMode::ShowVisibleFront) {
        rhi.SetDepthBias(0.0f, 0.0f);
    }
}
/*
void RenderBackEnd::DrawDebugNormals(int mode) const {
    cDrawVert *vptr;
    float drawLength;
    //	float distanceScale;
    //	Vec3 vec;
    Vec3 end;
    int i;

    rhi.SetStateBits(ColorWrite | DF_LEqual | DepthWrite | PM_Wireframe);

    switch (mode) {
    case 1:
        rhi.SetDepthBias(0.0f, -500.0f);
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
        //vec = m_entity->GetWorldMatrix() * vptr->xyz;
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
        rhi.SetDepthBias(0.0f, 0.0f);
        break;
    default:
        bglDepthRange(0.0, 1.0);
        break;
    }
}

void RenderBackEnd::DrawDebugTangents(int mode) const {
    cDrawVert *vptr;
    float drawLength;
    Vec3 tan, bitan;
    //	float distanceScale;
    //	Vec3 vec;
    Vec3 end;
    int i;

    rhi.SetStateBits(ColorWrite | DF_LEqual | DepthWrite | PM_Wireframe);

    switch (mode) {
    case 1:
        rhi.SetDepthBias(0.0f, -500.0f);
        break;
    default:
        bglDepthRange(0.0, 0.0);
        break;
    }

    //	distanceScale = cMath::Tan(m_view.m_fovX * 0.5f) / 100.0f;

    bglBegin(GL_LINES);

    vptr = (cDrawVert *)m_dp.systemVb;
    for (i = 0; i < numVerts; i++, vptr++) {
        //vec = m_entity->GetWorldMatrix() * vptr->xyz;
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
        rhi.SetDepthBias(0.0f, 0.0f);
        break;
    default:
        bglDepthRange(0.0, 1.0);
        break;
    }
}

void RenderBackEnd::DrawDebugTangentSpace(int tangentIndex) const {
    GL_BindBuffer(BGL_VERTEX_BUFFER, vertexBuffer);

    bglColor4ub(255, 255, 255, 255);

    rhi.SetStateBits(ColorWrite | DF_LEqual);

    rhi.SetCullFace((bglCullType_t)m_material->m_cullType);

    g_rsd.showTangentSpaceProg->Bind();
    g_rsd.showTangentSpaceProg->SetParameter1i("tangentIndex", tangentIndex);

    GL_SelectTexture(0);
    GL_ArrayOffset(BGL_TEXTURE_COORD_ARRAY, 4, BGL_FLOAT, false, sizeof(cDrawVert), OFFSET_OF(cDrawVert, tangent));

    bglEnableClientState(GL_NORMAL_ARRAY);
    GL_ArrayOffset(BGL_NORMAL_ARRAY, 3, BGL_FLOAT, false, sizeof(cDrawVert), OFFSET_OF(cDrawVert, normal));

    GL_ArrayOffset(BGL_VERTEX_ARRAY, 3, BGL_FLOAT, false, sizeof(cDrawVert), 0);

    DrawPrimitives();
}

void RenderBackEnd::DrawDebugBatch(const byte *rgb) const {
    GL_BindShader(BGL_NULL_SHADER);

    GL_BindBuffer(BGL_VERTEX_BUFFER, vertexBuffer);

    //g_textureManager.m_whiteTexture->Bind();

    bglColor3ubv(rgb);

    rhi.SetStateBits(ColorWrite | DF_Equal);
    rhi.SetCullFace((bglCullType_t)m_material->m_cullType);

    bglDisableClientState(GL_TEXTURE_COORD_ARRAY);

    GL_ArrayOffset(BGL_VERTEX_ARRAY, 3, BGL_FLOAT, false, sizeof(cDrawVert), 0);

    DrawPrimitives();
}*/

/*
void RenderBackEnd::RenderFogSurface(const volumeFog_t *fog) {
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
    if (!m_material->fog && (m_material->GetSort() >= Material::Sort::Overlay)) {
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
                c = cVec3_Dot(diff, m_view.m_viewaxis[AxisIndex::Forward]) / fogMaterial->fogDistance;
            } else { // 시점자가 포그 밖이라면
                diff = v - m_view.m_vieworg;
                c = cVec3_Dot(diff, m_view.m_viewaxis[AxisIndex::Forward]) / fogMaterial->fogDistance;
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
        rhi.SetStateBits(ColorWrite | DF_LEqual | BS_SrcAlpha | BD_OneMinusSrcAlpha);
    } else {
        rhi.SetStateBits(ColorWrite | DF_Equal | BS_SrcAlpha | BD_OneMinusSrcAlpha);
    }

    DrawPrimitives();
}

void RenderBackEnd::ModifyColorsForFog(const stage_t *pass) {
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
            c = cVec3_Dot(diff, m_view.m_viewaxis[AxisIndex::Forward]) / fogShader->fogDistance;
            c = 1.f - max(0.f, min(c, 1.f));
        } else { // 시점자가 포그 밖
            // 포그 평면과 월드버텍스와의 거리
            dist2 = cVec3_Dot(v, fogPlane->normal) - fogPlane->dist;

            if (dist2 < 0) {
                cVec3_Subtract(v, m_view.m_vieworg, diff);
                c = cVec3_Dot(diff, m_view.m_viewaxis[AxisIndex::Forward]) / fogShader->fogDistance;
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
