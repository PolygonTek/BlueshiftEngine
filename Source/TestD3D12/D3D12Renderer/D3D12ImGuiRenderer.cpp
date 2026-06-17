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
#include "D3D12ImGuiRenderer.h"
#include "D3D12Buffer.h"
#include "D3D12RootBindingData.h"
#include "D3D12CommandList.h"
#include "D3D12ImGuiOverlay.h"
#include "D3D12Renderer.h"
#include "D3D12SwapChain.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_dx12.h"
#include <mutex>
#include <unordered_map>
#include <vector>

struct ImGuiSrvDescriptorRecord {
    D3D12ImGuiOverlay *         owner = nullptr;
    uint32_t                    bindlessIndex = D3D12BindlessDescriptorAllocator::NullIndex;
};

// Keep this layout compatible with ImGui_ImplDX12_Texture. Official shutdown may destroy textures
// that were created by this backend while the official backend is still used for context bookkeeping.
struct ImGuiD3D12Texture {
    ID3D12Resource *            textureResource = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuDescriptorHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuDescriptorHandle = {};
};

static_assert(sizeof(ImGuiD3D12Texture) == sizeof(ID3D12Resource *) + sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE),
    "ImGuiD3D12Texture must stay layout-compatible with ImGui_ImplDX12_Texture");

static std::mutex               imguiSrvDescriptorMutex;
static std::unordered_map<SIZE_T, ImGuiSrvDescriptorRecord> imguiSrvDescriptorMap;
static ImDrawCallback           imguiResetRenderStateCallback = nullptr;
static ImDrawCallback           imguiSetSamplerLinearCallback = nullptr;
static ImDrawCallback           imguiSetSamplerNearestCallback = nullptr;
static constexpr uint32_t       LinearOutputPipelineStateKeyBit = 0x80000000u;

static bool RequiresLinearOutput(DXGI_FORMAT rtvFormat) {
    return D3D12Renderer::IsSRGBFormat(rtvFormat) || D3D12Renderer::IsHDRSwapChainFormat(rtvFormat);
}

static uint32_t MakePipelineStateKey(DXGI_FORMAT rtvFormat, bool linearOutput) {
    uint32_t key = static_cast<uint32_t>(rtvFormat);
    if (linearOutput) {
        key |= LinearOutputPipelineStateKeyBit;
    }
    return key;
}

static D3D12ImGuiOverlay *GetImGuiOverlay(ImGui_ImplDX12_InitInfo *initInfo) {
    return initInfo ? static_cast<D3D12ImGuiOverlay *>(initInfo->UserData) : nullptr;
}

static void AllocImGuiSrvDescriptor(D3D12ImGuiOverlay *owner, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle, uint32_t *outBindlessIndex = nullptr) {
    D3D12Renderer *renderer = D3D12Renderer::GetRenderer();
    const uint32_t bindlessIndex = renderer->AllocBindlessResourceDescriptor();
    if (bindlessIndex == D3D12BindlessDescriptorAllocator::NullIndex) {
        BE_FATALERROR("AllocImGuiSrvDescriptor: out of bindless resource descriptors");
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = renderer->GetBindlessResourceCPUDescriptorHandle(bindlessIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle = renderer->GetBindlessResourceGPUDescriptorHandle(bindlessIndex);

    {
        std::lock_guard<std::mutex> lock(imguiSrvDescriptorMutex);
        imguiSrvDescriptorMap[cpuDescriptorHandle.ptr] = { owner, bindlessIndex };
    }

    *outCpuDescriptorHandle = cpuDescriptorHandle;
    *outGpuDescriptorHandle = gpuDescriptorHandle;
    if (outBindlessIndex) {
        *outBindlessIndex = bindlessIndex;
    }
}

static void FreeImGuiSrvDescriptor(D3D12ImGuiOverlay *owner, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle) {
    UNREFERENCED_PARAMETER(gpuDescriptorHandle);

    uint32_t bindlessIndex = D3D12BindlessDescriptorAllocator::NullIndex;
    {
        std::lock_guard<std::mutex> lock(imguiSrvDescriptorMutex);

        auto it = imguiSrvDescriptorMap.find(cpuDescriptorHandle.ptr);
        if (it == imguiSrvDescriptorMap.end()) {
            BE_WARNLOG("FreeImGuiSrvDescriptor: descriptor handle was not allocated by ImGui\n");
            return;
        }

        if (it->second.owner != owner) {
            BE_WARNLOG("FreeImGuiSrvDescriptor: descriptor owner mismatch\n");
        }

        bindlessIndex = it->second.bindlessIndex;
        imguiSrvDescriptorMap.erase(it);
    }

    D3D12Renderer::GetRenderer()->FreeBindlessResourceDescriptor(bindlessIndex);
}

static void AllocImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo *initInfo, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle) {
    AllocImGuiSrvDescriptor(GetImGuiOverlay(initInfo), outCpuDescriptorHandle, outGpuDescriptorHandle);
}

static void FreeImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo *initInfo, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle) {
    FreeImGuiSrvDescriptor(GetImGuiOverlay(initInfo), cpuDescriptorHandle, gpuDescriptorHandle);
}

static int FreeRemainingImGuiSrvDescriptors(D3D12ImGuiOverlay *owner) {
    std::vector<uint32_t> bindlessIndices;

    {
        std::lock_guard<std::mutex> lock(imguiSrvDescriptorMutex);
        for (auto it = imguiSrvDescriptorMap.begin(); it != imguiSrvDescriptorMap.end();) {
            if (it->second.owner == owner) {
                bindlessIndices.push_back(it->second.bindlessIndex);
                it = imguiSrvDescriptorMap.erase(it);
            } else {
                ++it;
            }
        }
    }

    D3D12Renderer *renderer = D3D12Renderer::GetRenderer();
    for (uint32_t bindlessIndex : bindlessIndices) {
        renderer->FreeBindlessResourceDescriptor(bindlessIndex);
    }

    return static_cast<int>(bindlessIndices.size());
}

static uint32_t GetImGuiSrvDescriptorBindlessIndex(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle) {
    std::lock_guard<std::mutex> lock(imguiSrvDescriptorMutex);

    auto it = imguiSrvDescriptorMap.find(cpuDescriptorHandle.ptr);
    return it != imguiSrvDescriptorMap.end() ? it->second.bindlessIndex : D3D12BindlessDescriptorAllocator::NullIndex;
}

bool D3D12ImGuiRenderer::Init() {
    initialized = true;
    return true;
}

void D3D12ImGuiRenderer::Shutdown() {
    FreePipelineObjects();
    initialized = false;
}

bool D3D12ImGuiRenderer::CreatePipelineObjects(DXGI_FORMAT rtvFormat, bool linearOutput) {
    if (!CreateRootSignature()) {
        return false;
    }

    return CreatePipelineState(rtvFormat, linearOutput);
}

bool D3D12ImGuiRenderer::CreateRootSignature() {
    if (rootSignature) {
        return true;
    }

    D3D12Renderer *renderer = D3D12Renderer::GetRenderer();
    if (!renderer) {
        return false;
    }

    D3D12_DESCRIPTOR_RANGE descRange = {};
    descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descRange.NumDescriptors = 1;
    descRange.BaseShaderRegister = 0;
    descRange.RegisterSpace = 0;
    descRange.OffsetInDescriptorsFromTableStart = 0;

    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].Constants.ShaderRegister = 0;
    rootParameters[0].Constants.RegisterSpace = 0;
    rootParameters[0].Constants.Num32BitValues = 16;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &descRange;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.MipLODBias = 0.0f;
    staticSampler.MaxAnisotropy = 0;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler.MinLOD = 0.0f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0;
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = COUNT_OF(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &staticSampler;
    rootSignatureDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    ID3DBlob *rootSignatureBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob);
    if (errorBlob) {
        if (errorBlob->GetBufferSize() > 0) {
            BE_WARNLOG("%s\n", static_cast<const char *>(errorBlob->GetBufferPointer()));
        }
        errorBlob->Release();
    }
    if (FAILED(hr) || !rootSignatureBlob) {
        BE_WARNLOG("D3D12ImGuiRenderer::CreateRootSignature: D3D12SerializeRootSignature failed, ERROR: 0x%x\n", hr);
        return false;
    }

    hr = renderer->GetDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    rootSignatureBlob->Release();
    if (FAILED(hr)) {
        BE_WARNLOG("D3D12ImGuiRenderer::CreateRootSignature: CreateRootSignature failed, ERROR: 0x%x\n", hr);
        return false;
    }

    return true;
}

static bool CompileImGuiShader(const char *sourceName, const char *shaderText, const char *entryPoint, const char *target, ID3DBlob **outBlob) {
    ID3DBlob *errorBlob = nullptr;
    HRESULT hr = D3DCompile(shaderText, strlen(shaderText), sourceName, nullptr, nullptr, entryPoint, target, 0, 0, outBlob, &errorBlob);
    if (errorBlob) {
        if (errorBlob->GetBufferSize() > 0) {
            BE_WARNLOG("%s\n", static_cast<const char *>(errorBlob->GetBufferPointer()));
        }
        errorBlob->Release();
    }
    return SUCCEEDED(hr) && *outBlob;
}


bool D3D12ImGuiRenderer::CreatePipelineState(DXGI_FORMAT rtvFormat, bool linearOutput) {
    if (rtvFormat == DXGI_FORMAT_UNKNOWN) {
        BE_WARNLOG("D3D12ImGuiRenderer::CreatePipelineState: invalid RTV format\n");
        return false;
    }

    const uint32_t pipelineStateKey = MakePipelineStateKey(rtvFormat, linearOutput);
    if (pipelineStateMap.find(pipelineStateKey) != pipelineStateMap.end()) {
        return true;
    }

    if (!rootSignature) {
        return false;
    }

    D3D12Renderer *renderer = D3D12Renderer::GetRenderer();
    if (!renderer) {
        return false;
    }

    static const char *vertexShader =
        "cbuffer vertexBuffer : register(b0)"
        "{"
        "  float4x4 ProjectionMatrix;"
        "};"
        "struct VS_INPUT"
        "{"
        "  float2 pos : POSITION;"
        "  float4 col : COLOR0;"
        "  float2 uv  : TEXCOORD0;"
        "};"
        "struct PS_INPUT"
        "{"
        "  float4 pos : SV_POSITION;"
        "  float4 col : COLOR0;"
        "  float2 uv  : TEXCOORD0;"
        "};"
        "PS_INPUT main(VS_INPUT input)"
        "{"
        "  PS_INPUT output;"
        "  output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.0f, 1.0f));"
        "  output.col = input.col;"
        "  output.uv = input.uv;"
        "  return output;"
        "}";

    static const char *pixelShaderUnmodified =
        "struct PS_INPUT"
        "{"
        "  float4 pos : SV_POSITION;"
        "  float4 col : COLOR0;"
        "  float2 uv  : TEXCOORD0;"
        "};"
        "SamplerState sampler0 : register(s0);"
        "Texture2D texture0 : register(t0);"
        "float4 main(PS_INPUT input) : SV_Target"
        "{"
        "  return input.col * texture0.Sample(sampler0, input.uv);"
        "}";

    static const char *pixelShaderLinearOutput =
        "struct PS_INPUT"
        "{"
        "  float4 pos : SV_POSITION;"
        "  float4 col : COLOR0;"
        "  float2 uv  : TEXCOORD0;"
        "};"
        "SamplerState sampler0 : register(s0);"
        "Texture2D texture0 : register(t0);"
        "float3 SRGBToLinear(float3 color)"
        "{"
        "  return pow(abs(color), 2.2);"
        "}"
        "float4 main(PS_INPUT input) : SV_Target"
        "{"
        "  float4 color = input.col * texture0.Sample(sampler0, input.uv);"
        "  color.rgb = SRGBToLinear(color.rgb);"
        "  return color;"
        "}";

    const char *pixelShader = linearOutput ? pixelShaderLinearOutput : pixelShaderUnmodified;

    ID3DBlob *vertexShaderBlob = nullptr;
    ID3DBlob *pixelShaderBlob = nullptr;
    if (!CompileImGuiShader("ImGuiDX12VS", vertexShader, "main", "vs_5_0", &vertexShaderBlob)) {
        return false;
    }
    if (!CompileImGuiShader("ImGuiDX12PS", pixelShader, "main", "ps_5_0", &pixelShaderBlob)) {
        vertexShaderBlob->Release();
        return false;
    }

    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)offsetof(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)offsetof(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)offsetof(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
    psoDesc.InputLayout = { inputElements, COUNT_OF(inputElements) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = rtvFormat;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;

    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = FALSE;
    psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
    psoDesc.RasterizerState.ForcedSampleCount = 0;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DepthStencilState.FrontFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    psoDesc.DepthStencilState.BackFace = psoDesc.DepthStencilState.FrontFace;

    ID3D12PipelineState *newPipelineState = nullptr;
    HRESULT hr = renderer->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&newPipelineState));
    vertexShaderBlob->Release();
    pixelShaderBlob->Release();
    if (FAILED(hr)) {
        BE_WARNLOG("D3D12ImGuiRenderer::CreatePipelineState: CreateGraphicsPipelineState failed, ERROR: 0x%x\n", hr);
        return false;
    }

    pipelineStateMap.emplace(pipelineStateKey, newPipelineState);
    return true;
}

void D3D12ImGuiRenderer::FreePipelineObjects() {
    for (auto &entry : pipelineStateMap) {
        SAFE_RELEASE(entry.second);
    }
    pipelineStateMap.clear();
    SAFE_RELEASE(rootSignature);
}

ID3D12PipelineState *D3D12ImGuiRenderer::GetPipelineState(DXGI_FORMAT rtvFormat, bool linearOutput) const {
    auto it = pipelineStateMap.find(MakePipelineStateKey(rtvFormat, linearOutput));
    return it != pipelineStateMap.end() ? it->second : nullptr;
}

// 캡쳐 데이터의 텍스쳐를 처리하는 ImGui 내부 함수
bool D3D12ImGuiRenderer::UpdateTexture(D3D12ImGuiOverlay *overlay, ImTextureData *textureData) {
    if (!overlay || !textureData) {
        return false;
    }

    D3D12Renderer *renderer = D3D12Renderer::GetRenderer();
    if (!renderer) {
        return false;
    }

    bool needBarrierBeforeCopy = true;

    if (textureData->Status == ImTextureStatus_WantCreate) {
        if (textureData->TexID != ImTextureID_Invalid || textureData->BackendUserData != nullptr) {
            BE_WARNLOG("D3D12ImGuiRenderer::UpdateTexture: invalid create request state\n");
            return false;
        }
        if (textureData->Format != ImTextureFormat_RGBA32) {
            BE_WARNLOG("D3D12ImGuiRenderer::UpdateTexture: only RGBA32 ImGui textures are supported\n");
            return false;
        }

        ImGuiD3D12Texture *backendTexture = IM_NEW(ImGuiD3D12Texture)();
        uint32_t bindlessIndex = D3D12BindlessDescriptorAllocator::NullIndex;
        AllocImGuiSrvDescriptor(overlay, &backendTexture->srvCpuDescriptorHandle, &backendTexture->srvGpuDescriptorHandle, &bindlessIndex);

        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Width = textureData->Width;
        textureDesc.Height = textureData->Height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = renderer->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&backendTexture->textureResource));
        if (FAILED(hr)) {
            BE_WARNLOG("D3D12ImGuiRenderer::UpdateTexture: CreateCommittedResource failed, ERROR: 0x%x\n", hr);
            FreeImGuiSrvDescriptor(overlay, backendTexture->srvCpuDescriptorHandle, backendTexture->srvGpuDescriptorHandle);
            IM_DELETE(backendTexture);
            return false;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        renderer->GetDevice()->CreateShaderResourceView(backendTexture->textureResource, &srvDesc, backendTexture->srvCpuDescriptorHandle);

        textureData->SetTexID((ImTextureID)(UINT64)bindlessIndex);
        textureData->BackendUserData = backendTexture;
        needBarrierBeforeCopy = false;
    }

    if (textureData->Status == ImTextureStatus_WantCreate || textureData->Status == ImTextureStatus_WantUpdates) {
        ImGuiD3D12Texture *backendTexture = static_cast<ImGuiD3D12Texture *>(textureData->BackendUserData);
        if (!backendTexture || !backendTexture->textureResource) {
            return false;
        }
        if (textureData->Format != ImTextureFormat_RGBA32) {
            BE_WARNLOG("D3D12ImGuiRenderer::UpdateTexture: only RGBA32 ImGui textures are supported\n");
            return false;
        }

        const int uploadX = (textureData->Status == ImTextureStatus_WantCreate) ? 0 : textureData->UpdateRect.x;
        const int uploadY = (textureData->Status == ImTextureStatus_WantCreate) ? 0 : textureData->UpdateRect.y;
        const int uploadW = (textureData->Status == ImTextureStatus_WantCreate) ? textureData->Width : textureData->UpdateRect.w;
        const int uploadH = (textureData->Status == ImTextureStatus_WantCreate) ? textureData->Height : textureData->UpdateRect.h;
        if (uploadW <= 0 || uploadH <= 0) {
            textureData->SetStatus(ImTextureStatus_OK);
            return true;
        }

        const UINT uploadPitchSrc = uploadW * textureData->BytesPerPixel;
        const UINT uploadPitchDst = (uploadPitchSrc + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
        const UINT uploadSize = uploadPitchDst * uploadH;

        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = uploadSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ID3D12Resource *uploadBuffer = nullptr;
        HRESULT hr = renderer->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
        if (FAILED(hr)) {
            BE_WARNLOG("D3D12ImGuiRenderer::UpdateTexture: upload buffer CreateCommittedResource failed, ERROR: 0x%x\n", hr);
            return false;
        }

        void *mappedPtr = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        hr = uploadBuffer->Map(0, &readRange, &mappedPtr);
        if (FAILED(hr)) {
            uploadBuffer->Release();
            BE_WARNLOG("D3D12ImGuiRenderer::UpdateTexture: upload buffer Map failed, ERROR: 0x%x\n", hr);
            return false;
        }

        for (int y = 0; y < uploadH; ++y) {
            memcpy(static_cast<byte *>(mappedPtr) + y * uploadPitchDst, textureData->GetPixelsAt(uploadX, uploadY + y), uploadPitchSrc);
        }

        D3D12_RANGE writeRange = { 0, uploadSize };
        uploadBuffer->Unmap(0, &writeRange);

        D3D12CommandList *commandList = static_cast<D3D12CommandList *>(renderer->BeginUtilityCommandList(RHI::CommandQueueType::Graphics));
        if (!commandList) {
            uploadBuffer->Release();
            BE_WARNLOG("D3D12ImGuiRenderer::UpdateTexture: failed to get utility command list\n");
            return false;
        }

        if (needBarrierBeforeCopy) {
            commandList->ResourceBarrier(backendTexture->textureResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        }

        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.pResource = uploadBuffer;
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srcLocation.PlacedFootprint.Footprint.Width = uploadW;
        srcLocation.PlacedFootprint.Footprint.Height = uploadH;
        srcLocation.PlacedFootprint.Footprint.Depth = 1;
        srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitchDst;

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.pResource = backendTexture->textureResource;
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = 0;

        commandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, uploadX, uploadY, 0, &srcLocation, nullptr);
        commandList->ResourceBarrier(backendTexture->textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->CloseAndExecute();

        renderer->Finish(RHI::CommandQueueType::Graphics);
        renderer->EndUtilityCommandList(commandList);
        uploadBuffer->Release();

        textureData->SetStatus(ImTextureStatus_OK);
        return true;
    }

    if (textureData->Status == ImTextureStatus_WantDestroy && textureData->UnusedFrames >= D3D12SwapChain::NumSwapChainBuffers) {
        ImGuiD3D12Texture *backendTexture = static_cast<ImGuiD3D12Texture *>(textureData->BackendUserData);
        if (backendTexture) {
            const uint32_t bindlessIndex = GetImGuiSrvDescriptorBindlessIndex(backendTexture->srvCpuDescriptorHandle);
            if (bindlessIndex != static_cast<uint32_t>((UINT64)textureData->TexID)) {
                BE_WARNLOG("D3D12ImGuiRenderer::DestroyTexture: texture bindless index mismatch\n");
            }
            FreeImGuiSrvDescriptor(overlay, backendTexture->srvCpuDescriptorHandle, backendTexture->srvGpuDescriptorHandle);
            SAFE_RELEASE(backendTexture->textureResource);
            backendTexture->srvCpuDescriptorHandle.ptr = 0;
            backendTexture->srvGpuDescriptorHandle.ptr = 0;
            IM_DELETE(backendTexture);

            textureData->SetTexID(ImTextureID_Invalid);
            textureData->BackendUserData = nullptr;
        }
        textureData->SetStatus(ImTextureStatus_Destroyed);
    }

    return true;
}

bool D3D12ImGuiRenderer::InitOverlay(D3D12ImGuiOverlay *overlay, RHI::SwapChain *swapChain) {
    if (!initialized || !overlay || !swapChain) {
        return false;
    }

    D3D12Renderer *renderer = D3D12Renderer::GetRenderer();
    if (!renderer || !renderer->IsInitialized()) {
        BE_WARNLOG("D3D12ImGuiRenderer::InitOverlay: D3D12 renderer is not initialized\n");
        return false;
    }

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = renderer->GetDevice();
    initInfo.CommandQueue = renderer->commandQueues[to_int(RHI::CommandQueueType::Graphics)];
    initInfo.NumFramesInFlight = D3D12SwapChain::NumSwapChainBuffers;
    initInfo.RTVFormat = D3D12Renderer::RHIFormatToDXGIFormat(swapChain->GetRTVFormat());
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.UserData = overlay;
    initInfo.SrvDescriptorHeap = renderer->resShaderVisibleDescriptorHeap.GetDescriptorHeap();
    initInfo.SrvDescriptorAllocFn = AllocImGuiSrvDescriptor;
    initInfo.SrvDescriptorFreeFn = FreeImGuiSrvDescriptor;

    if (!CreatePipelineObjects(initInfo.RTVFormat, RequiresLinearOutput(initInfo.RTVFormat))) {
        BE_WARNLOG("D3D12ImGuiRenderer::InitOverlay: failed to create ImGui device objects\n");
        return false;
    }

    if (!ImGui_ImplDX12_Init(&initInfo)) {
        BE_WARNLOG("D3D12ImGuiRenderer::InitOverlay: ImGui_ImplDX12_Init failed\n");
        return false;
    }

    ImGuiPlatformIO &platformIO = ImGui::GetPlatformIO();
    imguiResetRenderStateCallback = platformIO.DrawCallback_ResetRenderState;
    imguiSetSamplerLinearCallback = platformIO.DrawCallback_SetSamplerLinear;
    imguiSetSamplerNearestCallback = platformIO.DrawCallback_SetSamplerNearest;

    return true;
}

bool D3D12ImGuiRenderer::UpdateTextures(D3D12ImGuiOverlay *overlay, ImVector<ImTextureData *> *textures) {
    if (!initialized || !overlay || !textures) {
        return true;
    }

    for (ImTextureData *textureData : *textures) {
        if (textureData && textureData->Status != ImTextureStatus_OK && !UpdateTexture(overlay, textureData)) {
            return false;
        }
    }

    return true;
}

void D3D12ImGuiRenderer::ShutdownOverlay(D3D12ImGuiOverlay *overlay) {
    if (!initialized || !overlay) {
        return;
    }

    ImGui_ImplDX12_Shutdown();

    const int leakedDescriptorCount = FreeRemainingImGuiSrvDescriptors(overlay);
    if (leakedDescriptorCount > 0) {
        BE_WARNLOG("D3D12ImGuiRenderer::ShutdownOverlay: leaking %i ImGui descriptors\n", leakedDescriptorCount);
    }
}

static void SetupRenderState(D3D12CommandList *commandList, ID3D12RootSignature *rootSignature, ID3D12PipelineState *pipelineState, ImDrawData *drawData, const RHI::VertexBuffer *vertexBuffer, const RHI::IndexBuffer *indexBuffer) {
    ID3D12GraphicsCommandList *graphicsCommandList = commandList->GetGraphicsCommandList();

    float L = drawData->DisplayPos.x;
    float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float T = drawData->DisplayPos.y;
    float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
    float mvp[4][4] = {
        { 2.0f / (R - L), 0.0f,           0.0f, 0.0f },
        { 0.0f,           2.0f / (T - B), 0.0f, 0.0f },
        { 0.0f,           0.0f,           0.5f, 0.0f },
        { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
    };

    D3D12_VIEWPORT viewport = {};
    viewport.Width = drawData->DisplaySize.x * drawData->FramebufferScale.x;
    viewport.Height = drawData->DisplaySize.y * drawData->FramebufferScale.y;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    graphicsCommandList->RSSetViewports(1, &viewport);

    graphicsCommandList->SetPipelineState(pipelineState);
    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetVertexBuffer(0, vertexBuffer);
    commandList->SetIndexBuffer(indexBuffer);
    graphicsCommandList->SetGraphicsRoot32BitConstants(0, 16, mvp, 0);

    const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    graphicsCommandList->OMSetBlendFactor(blendFactor);
}

void D3D12ImGuiRenderer::RenderDrawData(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, RHI::ImGuiOverlay *overlay, void *userData) {
    if (!initialized || !frameThreadData || !commandList || !overlay) {
        return;
    }

    D3D12ImGuiOverlay *_overlay = static_cast<D3D12ImGuiOverlay *>(overlay);
    ImDrawData *drawData = _overlay->GetCapturedDrawData(userData);
    if (!drawData || !drawData->Valid || drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f || drawData->TotalVtxCount <= 0 || drawData->TotalIdxCount <= 0) {
        return;
    }

    DXGI_FORMAT rtvFormat = D3D12Renderer::RHIFormatToDXGIFormat(_overlay->GetRTVFormat());
    const bool linearOutput = RequiresLinearOutput(rtvFormat);
    ID3D12PipelineState *pipelineState = GetPipelineState(rtvFormat, linearOutput);
    if (!pipelineState) {
        if (!CreatePipelineObjects(rtvFormat, linearOutput)) {
            return;
        }
        pipelineState = GetPipelineState(rtvFormat, linearOutput);
    }
    if (!rootSignature || !pipelineState) {
        return;
    }

    RHI::VertexBuffer *vertexBuffer = frameThreadData->AllocVertex(sizeof(ImDrawVert), drawData->TotalVtxCount);
    RHI::IndexBuffer *indexBuffer = frameThreadData->AllocIndex(sizeof(ImDrawIdx), drawData->TotalIdxCount);
    if (!vertexBuffer || !indexBuffer || !vertexBuffer->mappedPtr || !indexBuffer->mappedPtr) {
        return;
    }

    ImDrawVert *vertexDst = static_cast<ImDrawVert *>(vertexBuffer->mappedPtr);
    ImDrawIdx *indexDst = static_cast<ImDrawIdx *>(indexBuffer->mappedPtr);
    for (const ImDrawList *drawList : drawData->CmdLists) {
        memcpy(vertexDst, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(indexDst, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vertexDst += drawList->VtxBuffer.Size;
        indexDst += drawList->IdxBuffer.Size;
    }

    D3D12Renderer *renderer = D3D12Renderer::GetRenderer();
    if (!renderer || !renderer->resShaderVisibleDescriptorHeap.GetDescriptorHeap()) {
        return;
    }

    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    ID3D12GraphicsCommandList *graphicsCommandList = _commandList->GetGraphicsCommandList();

    ID3D12DescriptorHeap *descriptorHeaps[] = {
        renderer->resShaderVisibleDescriptorHeap.GetDescriptorHeap()
    };
    _commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    SetupRenderState(_commandList, rootSignature, pipelineState, drawData, vertexBuffer, indexBuffer);

    int globalVtxOffset = 0;
    int globalIdxOffset = 0;
    ImVec2 clipOff = drawData->DisplayPos;
    ImVec2 clipScale = drawData->FramebufferScale;

    for (const ImDrawList *drawList : drawData->CmdLists) {
        for (int cmdIndex = 0; cmdIndex < drawList->CmdBuffer.Size; ++cmdIndex) {
            const ImDrawCmd *drawCmd = &drawList->CmdBuffer[cmdIndex];
            if (drawCmd->UserCallback) {
                if (drawCmd->UserCallback == imguiResetRenderStateCallback ||
                    drawCmd->UserCallback == imguiSetSamplerLinearCallback ||
                    drawCmd->UserCallback == imguiSetSamplerNearestCallback ||
                    drawCmd->UserCallback == ImDrawCallback_ResetRenderState) {
                    SetupRenderState(_commandList, rootSignature, pipelineState, drawData, vertexBuffer, indexBuffer);
                } else {
                    drawCmd->UserCallback(drawList, drawCmd);
                }
                continue;
            }

            ImVec2 clipMin((drawCmd->ClipRect.x - clipOff.x) * clipScale.x, (drawCmd->ClipRect.y - clipOff.y) * clipScale.y);
            ImVec2 clipMax((drawCmd->ClipRect.z - clipOff.x) * clipScale.x, (drawCmd->ClipRect.w - clipOff.y) * clipScale.y);
            if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) {
                continue;
            }

            D3D12_RECT scissorRect = {
                static_cast<LONG>(clipMin.x),
                static_cast<LONG>(clipMin.y),
                static_cast<LONG>(clipMax.x),
                static_cast<LONG>(clipMax.y)
            };
            graphicsCommandList->RSSetScissorRects(1, &scissorRect);

            const uint32_t bindlessIndex = static_cast<uint32_t>((UINT64)drawCmd->GetTexID());
            if (bindlessIndex == D3D12BindlessDescriptorAllocator::NullIndex) {
                continue;
            }

            D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = renderer->GetBindlessResourceGPUDescriptorHandle(bindlessIndex);
            graphicsCommandList->SetGraphicsRootDescriptorTable(1, textureHandle);
            graphicsCommandList->DrawIndexedInstanced(drawCmd->ElemCount, 1, drawCmd->IdxOffset + globalIdxOffset, drawCmd->VtxOffset + globalVtxOffset, 0);
        }

        globalIdxOffset += drawList->IdxBuffer.Size;
        globalVtxOffset += drawList->VtxBuffer.Size;
    }
}

void D3D12ImGuiRenderer::FreeDrawData(RHI::ImGuiOverlay *overlay, void *userData) {
    if (!overlay) {
        return;
    }

    static_cast<D3D12ImGuiOverlay *>(overlay)->FreeCapturedDrawData(userData);
}

void D3D12Renderer::RenderImGuiDrawData(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, RHI::ImGuiOverlay *overlay, void *userData) {
    if (!overlay) {
        return;
    }
    if (!imguiRenderer) {
        return;
    }

    imguiRenderer->RenderDrawData(frameThreadData, commandList, overlay, userData);
}

void D3D12Renderer::FreeImGuiDrawData(RHI::ImGuiOverlay *overlay, void *userData) {
    if (!overlay) {
        return;
    }
    if (!imguiRenderer) {
        static_cast<D3D12ImGuiOverlay *>(overlay)->FreeCapturedDrawData(userData);
        return;
    }

    imguiRenderer->FreeDrawData(overlay, userData);
}

bool D3D12Renderer::InitImGuiOverlayRenderer(D3D12ImGuiOverlay *overlay, RHI::SwapChain *swapChain) {
    if (!imguiRenderer) {
        return false;
    }
    return imguiRenderer->InitOverlay(overlay, swapChain);
}

void D3D12Renderer::ShutdownImGuiOverlayRenderer(D3D12ImGuiOverlay *overlay) {
    if (!imguiRenderer) {
        return;
    }
    imguiRenderer->ShutdownOverlay(overlay);
}

bool D3D12Renderer::UpdateImGuiOverlayTextures(D3D12ImGuiOverlay *overlay, ImVector<ImTextureData *> *textures) {
    if (!imguiRenderer) {
        return true;
    }
    return imguiRenderer->UpdateTextures(overlay, textures);
}
