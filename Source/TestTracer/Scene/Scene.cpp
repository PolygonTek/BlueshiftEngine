#include "Precompiled.h"
#include "Scene/Scene.h"
#include "Shading/Material.h"
#include "Geometry/BVH.h"
#include "Geometry/Mesh.h"
#include "Geometry/Model.h"
#include "Shading/Texture.h"

SceneManager::SceneManager() = default;

SceneManager::~SceneManager() {
    Shutdown();
}

Model *SceneManager::CreateModel() {
    auto model = std::make_unique<Model>();
    Model *result = model.get();
    models.push_back(std::move(model));
    return result;
}

void SceneManager::SetLightProbeTexture(std::unique_ptr<TextureLightProbe> texture) {
    lightProbe = std::move(texture);
}

void SceneManager::Init() {
    Shutdown();
}

void SceneManager::AddDirectionalLight(const BE1::Vec3 &dir, const BE1::Vec3 &color) {
    Light light{};
    BE1::Vec3 lightDir = dir;
    lightDir.Normalize();
    light.lightVec.x = lightDir.x;
    light.lightVec.y = lightDir.y;
    light.lightVec.z = lightDir.z;
    light.lightVec.w = 0;
    light.lightColor = color;
    lightList.Append(light);
}

void SceneManager::AddPointLight(const BE1::Vec3 &point, float intensity, const BE1::Vec3 &color) {
    Light light{};
    light.lightVec.x = point.x;
    light.lightVec.y = point.y;
    light.lightVec.z = point.z;
    light.lightVec.w = 1;
    light.lightColor = color;
    light.intensity = intensity;
    lightList.Append(light);
}

void SceneManager::AddModel(const BE1::Vec3 &position, const BE1::Angles &angles, const Model *model) {
    Shape shape{};
    shape.type = Shape::MODEL;
    shape.position = position;
    shape.axis = angles.ToMat3();
    shape.model = model;
    shapeList.Append(shape);
}

void SceneManager::AddPlane(const BE1::Plane &plane, const Material *material) {
    Shape shape{};
    shape.type = Shape::PLANE;
    shape.position = BE1::Vec3(0, 0, 0);
    shape.axis.SetIdentity();
    shape.plane = plane;
    shape.material = material;
    shapeList.Append(shape);
}

void SceneManager::AddSphere(const BE1::Sphere &sphere, const Material *material) {
    Shape shape{};
    shape.type = Shape::SPHERE;
    shape.position = BE1::Vec3(0, 0, 0);
    shape.axis.SetIdentity();
    shape.sphere = sphere;
    shape.material = material;
    shapeList.Append(shape);
}

void SceneManager::Shutdown() {
    shapeList.Clear();
    lightList.Clear();
    models.clear();
    materials.clear();
    lightProbe.reset();
}
