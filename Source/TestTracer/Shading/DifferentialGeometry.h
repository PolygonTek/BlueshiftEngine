#pragma once

#include "BlueshiftEngine.h"

class Material;

class DifferentialGeometry {
public:
    BE1::Vec3                       x;
    BE1::Vec3                       gn;
    BE1::Vec3                       sn;
    BE1::Vec2                       st;
    const Material *                material;
};
