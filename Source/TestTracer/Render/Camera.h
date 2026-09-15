#pragma once

#include "BlueshiftEngine.h"

// Runtime 좌표계: +X 전방, +Y 왼쪽, +Z 위.
enum {
    ForwardAxis,
    LeftAxis,
    UpAxis
};

class Camera {
public:
    void                            Init(float fov, float aspectRatio);

    BE1::Vec3                       GetOrigin() const { return origin; }
    void                            SetOrigin(const BE1::Vec3 &origin);

    void                            SetAxis(const BE1::Mat3 &axis);
                                    // Angles 인자 순서는 (roll, pitch, yaw).
    void                            SetAngles(const BE1::Angles &angles);

    void                            GetRay(float ndx, float ndy, BE1::Vec3 *origin, BE1::Vec3 *dir) const;

protected:
    float                           fov = 0.0f;
    float                           aspectRatio = 1.0f;
    float                           tan_half_fov = 0.0f;
    BE1::Vec3                       origin = BE1::Vec3::zero;
    BE1::Mat3                       axis = BE1::Mat3::identity;
};

class PinholeCamera : public Camera {
public:
    void                            SetFocalDistance(float d) { focalDistance = d; }
    void                            SetLensRadius(float r) { lensRadius = r; }
    void                            GetPinholeRay(float ndx, float ndy, float dx, float dy, BE1::Vec3 *origin, BE1::Vec3 *dir) const;

private:
    float                           focalDistance = 1.0f;
    float                           lensRadius = 0.0f;
};
