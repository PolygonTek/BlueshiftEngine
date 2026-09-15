#include "Precompiled.h"
#include "Render/Camera.h"

void Camera::Init(float fov, float aspectRatio) {
    this->fov = fov;
    this->aspectRatio = aspectRatio;
    this->tan_half_fov = tanf(fov * 0.5f);
}

void Camera::GetRay(float ndx, float ndy, BE1::Vec3 *origin, BE1::Vec3 *dir) const {
    BE1::Vec3 np = BE1::Vec3(1.0f, -tan_half_fov * ndx, tan_half_fov * ndy * aspectRatio);
    np.Normalize();
    *dir = axis * np;
    *origin = this->origin;
}

void Camera::SetOrigin(const BE1::Vec3 &origin) {
    this->origin = origin;
}

void Camera::SetAxis(const BE1::Mat3 &axis) {
    this->axis = axis;
}

void Camera::SetAngles(const BE1::Angles &angles) {
    SetAxis(angles.ToMat3());
}

void PinholeCamera::GetPinholeRay(float ndx, float ndy, float dx, float dy, BE1::Vec3 *origin, BE1::Vec3 *dir) const {
    BE1::Vec3 right = BE1::Vec3(0.0f, -1.0f, 0.0f);
    BE1::Vec3 up = BE1::Vec3(0.0f, 0.0f, 1.0f);
    float nearDistance = 1.0f;

    // near plane point
    BE1::Vec3 npp = BE1::Vec3(1.0f, -tan_half_fov * ndx, tan_half_fov * ndy * aspectRatio);

    // focal plane point
    BE1::Vec3 fpp = npp * focalDistance / nearDistance;

    // lens plane point
    BE1::Vec3 lpp = (right * dx + up * dy) * lensRadius;

    *dir = fpp - lpp;
    dir->Normalize();
    *dir = axis * (*dir);
    *origin = this->origin + axis * lpp;
}
