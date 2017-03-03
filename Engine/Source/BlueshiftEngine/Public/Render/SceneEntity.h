#pragma once

/*
-------------------------------------------------------------------------------

    SceneEntity

-------------------------------------------------------------------------------
*/

#include "Core/WStr.h"
#include "Math/Math.h"
#include "Containers/Array.h"

BE_NAMESPACE_BEGIN

struct viewEntity_t;
struct DbvtProxy;
class Mat3x4;
class Skeleton;
class Material;
class Skin;
class Mesh;
class Font;

class SceneEntity {
    friend class RenderWorld;

public:
    enum MaterialParm {
        RedParm,
        GreenParm,
        BlueParm,
        AlphaParm,
        TimeOffsetParm,
        TimeScaleParm,
        MaxMaterialParms    // should be less than MAX_EXPR_LOCALPARMS
    };

    enum WireframeMode {
        ShowNone,
        ShowVisibleFront,
        ShowAllFront,
        ShowAllFrontAndBack
    };

    enum TextAnchor {
        UpperLeft,
        UpperCenter,
        UpperRight,
        MiddleLeft,
        MiddleCenter,
        MiddleRight,
        LowerLeft,
        LowerCenter,
        LowerRight
    };

    enum TextAlignment {
        Left,
        Center,
        Right
    };

    struct Parms {
        int                 layer;
        Mesh *              mesh;
        Font *              font;
        WStr                text;
        TextAnchor          textAnchor;
        TextAlignment       textAlignment;
        float               textScale;
        float               lineSpacing;
        const Skeleton *    skeleton;
        Array<Material *>   customMaterials;
        Skin *              customSkin;
        float               materialParms[MaxMaterialParms];
        int                 numJoints;
        Mat3x4 *            joints;
        WireframeMode       wireframeMode;
        Color4              wireframeColor;

        Vec3                origin;
        Vec3                scale;
        Mat3                axis;
        AABB                aabb;                   // non-scaled AABB 
        float               maxVisDist;

        bool                firstPersonOnly;
        bool                thirdPersonOnly;
        bool                billboard;
        bool                depthHack;
        bool                useLightProbe;
        bool                castShadows;
        bool                receiveShadows;
        bool                occluder;               // for use in HOM
        bool                skipSelectionBuffer;
        bool                richText;
    };

    SceneEntity();
    ~SceneEntity();

    void                    Update(const Parms *parms);

    const AABB              GetAABB() const;

    const Mat4 &            GetModelMatrix() const { return modelMatrix; }
    
    const OBB &             GetWorldOBB() const { return worldOBB; }

    void                    DrawJoints() const;

    int                     index;
    Parms                   parms;
    bool                    firstUpdate;
    Vec3                    origin;
    OBB                     worldOBB;
    Mat4                    modelMatrix;
    Mat4                    motionBlurModelMatrix[2];
    int                     viewCount;
    viewEntity_t *          viewEntity;
    DbvtProxy *             proxy;
    int                     numMeshSurfProxies;
    DbvtProxy *             meshSurfProxies;            // mesh surf proxy for static sub mesh
};

BE_NAMESPACE_END
