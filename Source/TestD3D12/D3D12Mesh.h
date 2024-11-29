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

class D3D12Mesh {
public:
    virtual void                    InitMesh() = 0;
    virtual void                    FreeMesh() = 0;

    bool                            CreateShader(const char *shaderText, int shaderTextSize, ID3DBlob **compiledVertexShader, ID3DBlob **compiledPixelShader);
    bool                            CreateShaderFromFile(const char *shaderFilename, ID3DBlob **compiledVertexShader, ID3DBlob **compiledPixelShader);
    ID3D12PipelineState *           CreatePSO(ID3D12RootSignature *rootSignature, ID3DBlob *compiledVertexShader, ID3DBlob *compiledPixelShader, const D3D12_INPUT_LAYOUT_DESC &inputLayout);
    ID3D12PipelineState *           CreatePSO(ID3D12RootSignature *rootSignature, const char *shaderFilename, const D3D12_INPUT_LAYOUT_DESC &inputLayout);
};
