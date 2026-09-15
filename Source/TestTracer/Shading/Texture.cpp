#include "Precompiled.h"
#include "Shading/Texture.h"

BE1::Image *LoadTextureImage(const char *filename) {
    auto *image = new BE1::Image;
    if (!image->Load(filename)) {
        delete image;
        return nullptr;
    }
    return image;
}

static BE1::Vec4 BilinearSample(const BE1::Image *image, float s, float t,
    BE1::Image::SampleWrapMode wrapMode = BE1::Image::SampleWrapMode::Clamp) {
    if (!image) {
        return BE1::Vec4(1.0f);
    }
    return image->Sample2D(BE1::Vec2(s, t), wrapMode, wrapMode).ToVec4();
}

Texture2D::~Texture2D() {
    delete image;
}

BE1::Vec4 Texture2D::Sample(const BE1::Vec2 &st) const {
    const auto wrapMode = clamp ? BE1::Image::SampleWrapMode::Clamp : BE1::Image::SampleWrapMode::Repeat;
    return BilinearSample(image, st.x, st.y, wrapMode);
}

TextureLightProbe::~TextureLightProbe() {
    delete image;
}

BE1::Vec4 TextureLightProbe::Sample(const BE1::Vec3 &dir) const {
    if (!image) {
        return BE1::Vec4(1.0f);
    }
    const BE1::Vec2 uv = BE1::Image::DirectionToEquirectangularUV(dir);
    return image->Sample2D(uv, BE1::Image::SampleWrapMode::Repeat, BE1::Image::SampleWrapMode::Clamp, BE1::Image::SampleFilter::Bilinear).ToVec4();
}
