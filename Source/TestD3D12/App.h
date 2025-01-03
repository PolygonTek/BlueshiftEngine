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

class GameObject;
class TriangleMesh;
class CubeMesh;

class App {
public:
    void                            Init(HWND windowHandle);
    void                            Shutdown();

    void                            RunFrame(int elapsedMsec);
    void                            Render(int elapsedMsec);

    int                             GetElapsedMsec() const { return elapsedMsec; }

    void                            SetViewMatrix(const BE1::Mat3 &viewAxis, const BE1::Vec3 &viewOrigin, float *rowMajor4x4ViewMatrix) const;

    void                            UpdateCamera();

    void                            ClearGameObjects();

    void                            InitGameObjects();
    void                            InitTriangles();
    void                            InitCubes();

    void                            UpdateGameObjects();
    void                            UpdateTriangles();
    void                            UpdateCubes();

    BE1::Array<GameObject *>        gameObjects;

    std::shared_ptr<TriangleMesh>   triangleMesh;
    std::shared_ptr<CubeMesh>       cubeMesh;

    BE1::Mat4                       viewProjMatrix;

    int                             elapsedMsec = 0;
};

extern App                          app;
