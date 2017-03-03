#pragma once

#include "Component.h"

BE_NAMESPACE_BEGIN

class ComTransform;

class ComSpline : public Component {
public:
    OBJECT_PROTOTYPE(ComSpline);

    ComSpline();
    virtual ~ComSpline();

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Awake() override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

    float                   Length();

    Vec3                    GetCurrentOrigin(float time) const;
    Mat3                    GetCurrentAxis(float time) const;

protected:
    void                    UpdateCurve();
    void                    PropertyChanged(const char *classname, const char *propName);
    void                    PointTransformUpdated(const ComTransform *transform);

    Array<Guid>             pointGuids;
    Curve_Spline<Vec3> *    originCurve;
    Curve_Spline<Angles> *  anglesCurve;
    bool                    curveUpdated;
    bool                    loop;
};

BE_NAMESPACE_END
