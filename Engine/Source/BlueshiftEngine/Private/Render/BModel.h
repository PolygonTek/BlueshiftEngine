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

BE_NAMESPACE_BEGIN

#define BSKEL_IDENT     MAKE_FOURCC('B', 'E', 'S', '1')
#define BSKEL_VERSION   1

#define BMESH_IDENT     MAKE_FOURCC('B', 'E', 'M', '1')
#define BMESH_VERSION   1

#define BANIM_IDENT     MAKE_FOURCC('B', 'E', 'A', '1')
#define BANIM_VERSION   1

enum BAnimFlag {
    RootTranslationXY   = BIT(0),
    RootTranslationZ    = BIT(1),
    RootRotation        = BIT(2),
};

#pragma pack(1)

struct BSkelHeader {
    int32_t         ident;
    int32_t         version;
    uint32_t        numJoints;
    uint32_t        padding;
};

struct BJoint {
    char            name[64];
    int32_t         parentIndex;
};

struct BMeshHeader {
    int32_t         ident;
    int32_t         version;
    uint32_t        numJoints;
    uint32_t        numSurfs;
};

struct BMeshSurf {
    int32_t         materialIndex;
    uint32_t        numVerts;
    uint32_t        numIndexes;
    uint32_t        indexSize;
    uint32_t        maxWeights;
};

struct BMeshVert {
    Vec3            position;
    Vec2            texCoord;
    Vec3            normal;
    Vec3            tangent;
    Vec3            bitangent;
    byte            color[4];
};

struct BAnimHeader {
    int32_t         ident;
    int32_t         version;
    uint32_t        flags;
    uint32_t        numJoints;
    uint32_t        numFrames;
    uint32_t        numAnimatedComponents;
    uint32_t        animLength;
    uint32_t        maxCycleCount;
};

struct BAnimJoint {
    char            name[64];
    int32_t         parentIndex;
    int32_t         animBits;
    int32_t         firstComponent;
};

#pragma pack()

BE_NAMESPACE_END
