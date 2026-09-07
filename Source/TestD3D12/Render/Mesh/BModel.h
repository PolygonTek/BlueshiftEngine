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

#define BSKEL_IDENT     MAKE_FOURCC('B', 'E', 'S', '1')
#define BSKEL_VERSION   1

#define BMESH_IDENT     MAKE_FOURCC('B', 'E', 'M', '1')
#define BMESH_VERSION   2

#define BANIM_IDENT     MAKE_FOURCC('B', 'E', 'A', '1')
#define BANIM_VERSION   2

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
    char            name[60];
    int32_t         parentIndex;
};

struct BMeshHeader {
    int32_t         ident;
    int32_t         version;
    uint32_t        numJoints;
    uint32_t        numSurfs;
    BE1::Vec3       boundsMin;
    BE1::Vec3       boundsMax;
};

struct BMeshSurf {
    int32_t         materialIndex;
    uint32_t        numVerts;
    uint32_t        numIndexes;
    uint16_t        indexSize;
    uint16_t        maxWeights;
    BE1::Vec3       boundsMin;
    BE1::Vec3       boundsMax;
};

// Legacy v1 vertex layout; retained for reading existing bmesh files.
struct BMeshVertV1 {
    BE1::Vec3       position;
    BE1::Vec2       texCoord;
    BE1::Vec3       normal;
    BE1::Vec3       tangent;
    BE1::Vec3       bitangent;
    uint32_t        color;
    uint32_t        reserved;
};

// v2 preserves the runtime half UV and packed tangent-frame bits.
// Tangent frames use oct11/oct11/diamond9/sign1; changing this encoding requires a version bump.
struct BMeshVert {
    BE1::Vec3       position;
    uint16_t        texCoord[2];
    uint32_t        color;
    uint32_t        tangentFrame;
};

static_assert(sizeof(BMeshVertV1) == 64, "Unexpected bmesh v1 vertex layout");
static_assert(sizeof(BMeshVert) == 24, "Unexpected bmesh v2 vertex layout");

struct BAnimHeader {
    int32_t         ident;
    int32_t         version;
    uint32_t        flags;
    uint32_t        numJoints;
    uint32_t        numFrames;
    uint32_t        numComponentsPerFrame;
    uint32_t        length;
    uint32_t        maxCycleCount;
};

struct BAnimJoint {
    char            name[60];
    int32_t         parentIndex;
    int32_t         componentBits;
    int32_t         componentOffset;
};

#pragma pack()
