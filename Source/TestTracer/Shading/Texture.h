#pragma once

#include "BlueshiftEngine.h"

BE1::Image *LoadTextureImage(const char *filename);

class Texture {
public:
};

class Texture2D : public Texture {
public:
    Texture2D(const Texture2D &) = delete;
    Texture2D &operator=(const Texture2D &) = delete;
    Texture2D(const BE1::Image *image) { this->image = image; }
    ~Texture2D();

    BE1::Vec4                       Sample(const BE1::Vec2 &st) const;

private:
    const BE1::Image *              image;
    bool                            clamp = false;
};

class TextureLightProbe : public Texture {
public:
    TextureLightProbe(const TextureLightProbe &) = delete;
    TextureLightProbe &operator=(const TextureLightProbe &) = delete;
    TextureLightProbe(const BE1::Image *image) { this->image = image; }
    ~TextureLightProbe();

    BE1::Vec4                       Sample(const BE1::Vec3 &dir) const;

private:
    const BE1::Image *              image;
};
