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

#pragma once

BE_NAMESPACE_BEGIN

#define BSKEL_IDENT     MAKE_FOURCC('B', 'E', 'S', '1')
#define BSKEL_VERSION   1

#define BMESH_IDENT     MAKE_FOURCC('B', 'E', 'M', '1')
#define BMESH_VERSION   3

#define BANIM_IDENT     MAKE_FOURCC('B', 'E', 'A', '1')
#define BANIM_VERSION   2

enum BAnimFlag {
    RootTranslationXY   = BIT(0),
    RootTranslationZ    = BIT(1),
    RootRotation        = BIT(2),
};

#pragma pack(1)

struct BSkelHeader {
    int32_t             ident;
    int32_t             version;
    uint32_t            numJoints;
    uint32_t            padding;
};

struct BJoint {
    char                name[60];
    int32_t             parentIndex;
};

struct BMeshHeader {
    int32_t             ident;
    int32_t             version;
    uint32_t            numJoints;
    uint32_t            numSurfs;
    Vec3                aabbMin;
    Vec3                aabbMax;
};

struct BMeshSurf {
    int32_t             materialIndex;
    uint32_t            numVerts;
    uint32_t            numIndexes;
    uint16_t            indexSize;
    uint16_t            maxWeights;
    Vec3                aabbMin;
    Vec3                aabbMax;
};

// v3 에서는 BMeshSurf 바로 뒤에 저장된다. 데이터 순서는 정점, 가중치, 인덱스이다.
// 크기는 파일에 저장된 바이트 수이며, 해당 플래그가 꺼져 있으면 비압축 블록이다.
struct BMeshSurfCompression {
    enum {
        VertexBuffer    = BIT(0),
        IndexBuffer     = BIT(1)
    };

    uint32_t            flags;
    uint32_t            vertexDataSize;
    uint32_t            indexDataSize;
};

// 기존 bmesh 파일을 읽기 위해 유지하는 v1 vertex layout.
struct BMeshVertV1 {
    Vec3                position;
    Vec2                texCoord;
    Vec3                normal;
    Vec3                tangent;
    Vec3                bitangent;
    uint32_t            color;
    uint32_t            reserved;
};

// v2 와 v3 는 런타임의 half UV 와 packing 된 탄젠트 프레임 비트를 그대로 보존한다.
// 탄젠트 프레임은 oct11/oct11/diamond9/sign1 인코딩을 사용하며, 인코딩을 변경하면 포맷 버전도 올려야 한다.
struct BMeshVert {
    Vec3                position;
    uint16_t            texCoord[2];
    uint32_t            color;
    uint32_t            tangentFrame;
};

struct BAnimHeader {
    int32_t             ident;
    int32_t             version;
    uint32_t            flags;
    uint32_t            numJoints;
    uint32_t            numFrames;
    uint32_t            numComponentsPerFrame;
    uint32_t            length;
    uint32_t            maxCycleCount;
};

struct BAnimJoint {
    char                name[60];
    int32_t             parentIndex;
    int32_t             componentBits;
    int32_t             componentOffset;
};

#pragma pack()

BE_NAMESPACE_END
