#pragma once

#include "Shading/Material.h"
#include <memory>
#include <vector>
#include <utility>

#include "BlueshiftEngine.h"

class Material;
class Model;
class TextureLightProbe;

class Shape {
public:
    enum {
        PLANE,
        SPHERE,
        QUAD,
        MODEL,
    } type = PLANE;
    BE1::Plane                      plane;
    BE1::Sphere                     sphere;
    // Observers; resources are owned by the scene or the imported model.
    const Material *                material = nullptr;
    const Model *                   model = nullptr;
    BE1::Vec3                       position;
    BE1::Mat3                       axis;
};

class Light {
public:
    BE1::Vec4                       lightVec;
    BE1::Vec3                       lightColor;
    float                           intensity;
};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();
    SceneManager(const SceneManager &) = delete;
    SceneManager &operator=(const SceneManager &) = delete;

    void                            Init();

                                    // 여기서 생성한 모델과 재질은 SceneManager가 소유한다.
    Model *                         CreateModel();

    template <typename T, typename... Args>
    T *                             CreateMaterial(Args &&...args);
    void                            Shutdown();

    void                            AddDirectionalLight(const BE1::Vec3 &dir, const BE1::Vec3 &color);
    void                            AddPointLight(const BE1::Vec3 &point, float intensity, const BE1::Vec3 &color);
    void                            AddPlane(const BE1::Plane &plane, const Material *material);
    void                            AddSphere(const BE1::Sphere &sphere, const Material *material);
    void                            AddModel(const BE1::Vec3 &position, const BE1::Angles &angles, const Model *model);

                                    // Transfers exclusive ownership to the scene.
    void                            SetLightProbeTexture(std::unique_ptr<TextureLightProbe> texture);
    const TextureLightProbe *       GetLightProbeTexture() const { return lightProbe.get(); }
    const BE1::Array<Shape> &       GetShapes() const { return shapeList; }
    const BE1::Array<Light> &       GetLights() const { return lightList; }

private:
    BE1::Array<Shape>               shapeList;
    BE1::Array<Light>               lightList;
    std::vector<std::unique_ptr<Model>> models;
    std::vector<std::unique_ptr<Material>> materials;
    std::unique_ptr<TextureLightProbe> lightProbe;
};

template <typename T, typename... Args>
BE_INLINE T *SceneManager::CreateMaterial(Args &&...args) {
    auto material = std::make_unique<T>(std::forward<Args>(args)...);
    T *result = material.get();
    materials.push_back(std::move(material));
    return result;
}
