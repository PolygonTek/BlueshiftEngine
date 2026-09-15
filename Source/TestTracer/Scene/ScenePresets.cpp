#include "Precompiled.h"
#include "Scene/ScenePresets.h"
#include "Scene/Scene.h"
#include "Geometry/Model.h"
#include "Geometry/Mesh.h"
#include "Shading/Texture.h"
#include "Shading/Matte.h"
#include "Shading/Plastic.h"
#include "Shading/Metal.h"
#include "Shading/Mirror.h"
#include "Shading/Glass.h"

static constexpr char SkullModelPath[] = "Data/Test/Skull.obj";

class SceneBuilder {
public:
    SceneBuilder(SceneManager &scene, SceneView &view, int width, int height)
        : scene(scene), camera(view.camera), unitMeter(view.unitMeter), screenWidth(width), screenHeight(height) {}

    void BuildBuiltin();
    void BuildSkull();
    void BuildKilleroo();
    void BuildLegoCar();
    void BuildCornellBox();
    void BuildSponza();
    void BuildSibenik();
    void BuildOptimus();

private:
    void SetUnitMeter(float value) { unitMeter = value; }
    float GetUnitMeter() const { return unitMeter; }

    SceneManager &scene;
    PinholeCamera &camera;
    float &unitMeter;
    int screenWidth;
    int screenHeight;
};

void SceneBuilder::BuildKilleroo() {
    SetUnitMeter(40.0f);
    camera.SetFocalDistance(25.f);
    camera.SetLensRadius(0.01f * GetUnitMeter());

    camera.Init(DEG2RAD(60), (float)screenHeight / screenWidth);
    camera.SetOrigin(BE1::Vec3(0.0f, 35.0f, 14.0f));
    camera.SetAngles(BE1::Angles(90.0f, 0.0f, 11.0f));

    Model *floorModel = scene.CreateModel();
    SubMesh *mesh = floorModel->AllocSubMesh(4, 6);

    mesh->material = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.162f, 0.146f, 0.12f), 0.0f);
    mesh->verts[0].xyz = BE1::Vec3(22, -22, 0);
    mesh->verts[0].normal = BE1::Vec3(0, 0, 1);
    mesh->verts[0].st = BE1::Vec2(0, 0);
    mesh->verts[1].xyz = BE1::Vec3(22, 22, 0);
    mesh->verts[1].normal = BE1::Vec3(0, 0, 1);
    mesh->verts[1].st = BE1::Vec2(1, 0);
    mesh->verts[2].xyz = BE1::Vec3(-22, -22, 0);
    mesh->verts[2].normal = BE1::Vec3(0, 0, 1);
    mesh->verts[2].st = BE1::Vec2(0, 1);
    mesh->verts[3].xyz = BE1::Vec3(-22, 22, 0);
    mesh->verts[3].normal = BE1::Vec3(0, 0, 1);
    mesh->verts[3].st = BE1::Vec2(1, 1);

    mesh->indexes[0] = 0;
    mesh->indexes[1] = 1;
    mesh->indexes[2] = 2;
    mesh->indexes[3] = 2;
    mesh->indexes[4] = 1;
    mesh->indexes[5] = 3;

    floorModel->FinishMeshes();
    floorModel->BuildBVH();

    // obj model mesh
    Model *objModel = scene.CreateModel();
    objModel->LoadFromObj("Data/models/killeroo.obj");
    mesh = objModel->GetMesh(0);
    //mesh->material = scene.CreateMaterial<PlasticMaterial>(BE1::Vec3(0.20f, 0.0f, 0.0f), BE1::Vec3(1.0f, 1.0f, 1.0f), 0.001f);
    //mesh->material = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.55f, 0.55f, 0.55f), 0.8f);
    mesh->material = scene.CreateMaterial<MetalMaterial>(BE1::Vec3(0.998f, 0.998f, 0.998f), BE1::Vec3(1.8f, 1.324f, 0.432f), 0.003f);
    //mesh->material = scene.CreateMaterial<GlassMaterial>(BE1::Vec3(0.6f, 1.4f, 1.6f), BE1::Vec3(0.6f, 1.5f, 1.6f), 1.5f);
    //mesh->material = new cMirror(BE1::Vec3(1.0f, 1.0f, 1.0f));*/

    objModel->FinishMeshes();
    objModel->BuildBVH();

    scene.AddModel(BE1::Vec3(0, 0, 0), BE1::Angles(0, 0, 0), floorModel);
    scene.AddModel(BE1::Vec3(0, 0, 13.8f), BE1::Angles(-240, 0, 0), objModel);
    scene.AddSphere(BE1::Sphere(BE1::Vec3(8, 12, 2.8f), 2.8f), scene.CreateMaterial<MetalMaterial>(BE1::Vec3(0.998f, 0.998f, 0.998f), BE1::Vec3(1.5f, 1.5f, 1.5f), 0.01f));

    // light mesh
    /*Model *lightModel = scene.CreateModel();
    Material *lightMaterial = EmissiveMaterial(BE1::Vec3(20.0, 20.0f, 20.0f);
    SubMesh *mesh = lightModel->AllocSubMesh(4, 6);
    mesh->verts[0].xyz = BE1::Vec3(-3.0f, -3.0f, 0);
    mesh->verts[1].xyz = BE1::Vec3(-3.0f, 3.0f, 0);
    mesh->verts[2].xyz = BE1::Vec3(3.0f, -3.0f, 0);
    mesh->verts[3].xyz = BE1::Vec3(3.0f, 3.0f, 0);

    mesh->indexes[0] = 0;
    mesh->indexes[1] = 1;
    mesh->indexes[2] = 2;
    mesh->indexes[3] = 2;
    mesh->indexes[4] = 1;
    mesh->indexes[5] = 3;

    lightModel->FinishMeshes();
    lightModel->BuildBVH();

    scene.AddModel(BE1::Vec3(-40, 40, 100), BE1::Vec3(0, 0, 0), lightModel);*/
    //scene.AddDirectionalLight(BE1::Vec3(1.0f, 0.5f, 0.9f), BE1::Vec3(1.0f, 1.0f, 1.0f));

    BE1::Image *image = LoadTextureImage("Data/lightprobes/pisa.hdr");
    //BE1::Image *image = LoadTextureImage("lightprobes/ennis.hdr");
    //BE1::Image *image = LoadTextureImage("lightprobes/doge2.hdr");
    //BE1::Image *image = LoadTextureImage("lightprobes/uffizi-large.hdr");
    //BE1::Image *image = LoadTextureImage("lightprobes/glacier.hdr");
    //BE1::Image *image = LoadTextureImage("lightprobes/grace-new.hdr");
    //BE1::Image *image = LoadTextureImage("lightprobes/hallstatt4_hd.hdr");
    auto lightProbeTexture = std::make_unique<TextureLightProbe>(image);
    scene.SetLightProbeTexture(std::move(lightProbeTexture));
}

void SceneBuilder::BuildLegoCar() {
    //SetUnitMeter(20.0f);
    SetUnitMeter(1.0f);
    camera.SetFocalDistance(0.55f);
    camera.SetLensRadius(0.01f * GetUnitMeter());

    camera.Init(DEG2RAD(45), (float)screenHeight / screenWidth);
    //camera.SetOrigin(BE1::Vec3(-25.0f, 0.0f, 10.0f));
    //camera.SetOrigin(BE1::Vec3(0.3f, 0.0f, 0.18f));
    camera.SetOrigin(BE1::Vec3(0.563f, 0.0f, 0.24f));
    //camera.SetOrigin(BE1::Vec3(1.4f, 0.0f, 0.6f));
    //camera.SetOrigin(BE1::Vec3(8.0f, 0.0f, 3.0f));
    //camera.SetYawPitchRoll(0.0f, 12.0f, 0.0f);
    camera.SetAngles(BE1::Angles(180.0f, -14.0f, 0.0f));

    Model *objModel = scene.CreateModel();
    //objModel->LoadFromObj("models/dragon.obj");
    //objModel->LoadFromObj("models/horse/horse97k.obj");
    //objModel->LoadFromObj("models/happy/happy.obj");
    //objModel->LoadFromObj("models/hand/hand.obj");
    objModel->LoadFromObj("models/legocar/legocar.obj");
    //objModel->LoadFromObj("models/oldcar/oldcar.obj");
    objModel->BuildBVH();

    Model *lightModel = scene.CreateModel();
    SubMesh *mesh = lightModel->AllocSubMesh(4, 6);
    mesh->material = scene.CreateMaterial<EmissiveMaterial>(BE1::Vec3(8.0, 9.0f, 9.0f));

    mesh->verts[0].xyz = BE1::Vec3(-1.6f, -1.6f, 0);
    mesh->verts[1].xyz = BE1::Vec3(-1.6f, 1.6f, 0);
    mesh->verts[2].xyz = BE1::Vec3(1.6f, -1.6f, 0);
    mesh->verts[3].xyz = BE1::Vec3(1.6f, 1.6f, 0);

    mesh->indexes[0] = 0;
    mesh->indexes[1] = 1;
    mesh->indexes[2] = 2;
    mesh->indexes[3] = 2;
    mesh->indexes[4] = 1;
    mesh->indexes[5] = 3;

    lightModel->FinishMeshes();
    lightModel->BuildBVH();

    /*
    materialA = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.22f, 0.24f, 0.3f), 0.0f);
    materialB = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(1.0f, 0.9f, 0.5f), 0.2f);
    materialC = scene.CreateMaterial<PlasticMaterial>(BE1::Vec3(0.12f, 0.33f, 0.61f), BE1::Vec3(1.0f, 1.0f, 1.0f), 0.001f);
    materialD = new cMetal(BE1::Vec3(0.998f, 0.998f, 0.998f), BE1::Vec3(1.8f, 1.324f, 0.432f), 0.0032f);
    materialE = new cMetal(BE1::Vec3(0.998f, 0.998f, 0.998f), BE1::Vec3(1.5f, 1.5f, 1.5f), 0.05f);*/

    scene.AddPlane(BE1::Plane(BE1::Vec3(0, 0, 1), 0), scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.22f, 0.24f, 0.32f), 0.0f));
    scene.AddModel(BE1::Vec3(0, 0.04f, -objModel->GetAABB().b[0].z - 0.003f), BE1::Angles(120, -3.9f, 0), objModel);
    //scene.AddModel(BE1::Vec3(0, 0.04f, -objModel->GetAABB().b[0].z - 0.003f), BE1::Vec3(90, -3.9f, 0), objModel);
    //  scene.AddSphere(BE1::Sphere(BE1::Vec3(8, -8, 5.6f), 5.6f), materialB);
    //  scene.AddSphere(BE1::Sphere(BE1::Vec3(-8, 8, 3.6f), 3.6f), materialE);
    scene.AddModel(BE1::Vec3(2, 2, 5), BE1::Angles(0, 0, 0), lightModel);
    /*
    BE1::Image *image = new BE1::Image;
    image->LoadHDR("lightprobes/pisa.hdr");
    //image->LoadHDR("lightprobes/ennis.hdr");
    //image->LoadHDR("lightprobes/doge2.hdr");
    //image->LoadHDR("lightprobes/uffizi-large.hdr");
    //image->LoadHDR("lightprobes/glacier.hdr");
    //image->LoadHDR("lightprobes/grace-new.hdr");
    //image->LoadHDR("lightprobes/hallstatt4_hd.hdr");
    auto lightProbeTexture = std::make_unique<TextureLightProbe>(image);
    scene.SetLightProbeTexture(std::move(lightProbeTexture));*/
}

void SceneBuilder::BuildCornellBox() {
    SetUnitMeter(400.0f);
    camera.SetFocalDistance(680.f);
    camera.SetLensRadius(0.01f * GetUnitMeter());

    camera.Init(DEG2RAD(50), (float)screenHeight / screenWidth);
    camera.SetOrigin(BE1::Vec3(680.0f, 0.0f, 200.0f));
    camera.SetAngles(BE1::Angles(180.0f, 0.0f, 0.0f));

    Model *testModel = scene.CreateModel();
    SubMesh *mesh = testModel->AllocSubMesh(8, 18);
    mesh->material = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.77f, 0.77f, 0.77f), 0.0f);
    mesh->verts[0].xyz = BE1::Vec3(200, -200, 0);
    mesh->verts[1].xyz = BE1::Vec3(200, 200, 0);
    mesh->verts[2].xyz = BE1::Vec3(-200, -200, 0);
    mesh->verts[3].xyz = BE1::Vec3(-200, 200, 0);
    mesh->verts[4].xyz = BE1::Vec3(-200, 200, 400);
    mesh->verts[5].xyz = BE1::Vec3(-200, -200, 400);
    mesh->verts[6].xyz = BE1::Vec3(200, -200, 400);
    mesh->verts[7].xyz = BE1::Vec3(200, 200, 400);

    mesh->indexes[0] = 0;
    mesh->indexes[1] = 1;
    mesh->indexes[2] = 2;
    mesh->indexes[3] = 2;
    mesh->indexes[4] = 1;
    mesh->indexes[5] = 3;
    mesh->indexes[6] = 3;
    mesh->indexes[7] = 4;
    mesh->indexes[8] = 2;
    mesh->indexes[9] = 2;
    mesh->indexes[10] = 4;
    mesh->indexes[11] = 5;
    mesh->indexes[12] = 5;
    mesh->indexes[13] = 4;
    mesh->indexes[14] = 6;
    mesh->indexes[15] = 6;
    mesh->indexes[16] = 4;
    mesh->indexes[17] = 7;

    mesh = testModel->AllocSubMesh(4, 6);
    mesh->material = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.77f, 0.0f, 0.0f), 0.0f);
    mesh->verts[0].xyz = BE1::Vec3(200, -200, 0);
    mesh->verts[1].xyz = BE1::Vec3(-200, -200, 0);
    mesh->verts[2].xyz = BE1::Vec3(200, -200, 400);
    mesh->verts[3].xyz = BE1::Vec3(-200, -200, 400);

    mesh->indexes[0] = 0;
    mesh->indexes[1] = 1;
    mesh->indexes[2] = 2;
    mesh->indexes[3] = 2;
    mesh->indexes[4] = 1;
    mesh->indexes[5] = 3;

    mesh = testModel->AllocSubMesh(4, 6);
    mesh->material = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.0f, 0.77f, 0.0f), 0.0f);
    mesh->verts[0].xyz = BE1::Vec3(-200, 200, 0);
    mesh->verts[1].xyz = BE1::Vec3(200, 200, 0);
    mesh->verts[2].xyz = BE1::Vec3(-200, 200, 400);
    mesh->verts[3].xyz = BE1::Vec3(200, 200, 400);

    mesh->indexes[0] = 0;
    mesh->indexes[1] = 1;
    mesh->indexes[2] = 2;
    mesh->indexes[3] = 2;
    mesh->indexes[4] = 1;
    mesh->indexes[5] = 3;

    mesh = testModel->AllocSubMesh(4, 6);
    mesh->material = scene.CreateMaterial<EmissiveMaterial>(BE1::Vec3(22.0f, 18.0f, 6.0f));
    mesh->verts[0].xyz = BE1::Vec3(-50.0f, -50.0f, 399.99f);
    mesh->verts[1].xyz = BE1::Vec3(-50.0f, 50.0f, 399.99f);
    mesh->verts[2].xyz = BE1::Vec3(50.0f, -50.0f, 399.99f);
    mesh->verts[3].xyz = BE1::Vec3(50.0f, 50.0f, 399.99f);

    mesh->indexes[0] = 0;
    mesh->indexes[1] = 1;
    mesh->indexes[2] = 2;
    mesh->indexes[3] = 2;
    mesh->indexes[4] = 1;
    mesh->indexes[5] = 3;

    testModel->FinishMeshes();
    testModel->BuildBVH();

    //  scene.AddPointLight(BE1::Vec3(1000, -1000, 1000), 2000.0f, BE1::Vec3(3, 3, 3));

    scene.AddModel(BE1::Vec3(0, 0, 0), BE1::Angles(0, 0, 0), testModel);

    /*materialD = new cMirror(BE1::Vec3(0.9f, 0.9f, 0.9f));*/
    Material *materialE = scene.CreateMaterial<PlasticMaterial>(BE1::Vec3(1.0f), BE1::Vec3(1.0f), 1.0f / 800.0f);
    Material *materialF = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(1.0f, 1.0f, 1.0f), 0.4f);
    Material *materialG = scene.CreateMaterial<GlassMaterial>(BE1::Vec3(0.9f, 0.9f, 0.9f), BE1::Vec3(0.9f, 0.9f, 0.9f), 2.5f);

    scene.AddSphere(BE1::Sphere(BE1::Vec3(-100, 100, 70.0f), 70.0f), materialE);
    scene.AddSphere(BE1::Sphere(BE1::Vec3(0, -100, 70.0f), 70.0f), materialG);
    //  scene.AddMesh(BE1::Vec3(30, 270, -68.0f), BE1::Vec3(0.0f, 0, 0), objModel, materialF);

    /*  Model *objModel = scene.CreateModel();
    objModel->LoadFromObj("models/smoothcube.obj");
    objModel->BuildBVH();
    scene.AddModel(BE1::Vec3(0, 0, 100), BE1::Vec3(10, 0, 0), objModel);*/
}

void SceneBuilder::BuildSponza() {
    SetUnitMeter(2.0f);
    camera.SetFocalDistance(100.f);
    camera.SetLensRadius(0.01f * GetUnitMeter());

    camera.Init(DEG2RAD(80), (float)screenHeight / screenWidth);
    camera.SetOrigin(BE1::Vec3(-1.0f, 14.1f, 7.0f));
    camera.SetAngles(BE1::Angles(82.0f, 0.0f, 0.0f));

    Model *objModel = scene.CreateModel();
    objModel->LoadFromObj("models/dabrovic-sponza/sponza.obj");
    objModel->BuildBVH();

    Model *lightModel = scene.CreateModel();
    SubMesh *mesh = lightModel->AllocSubMesh(4, 6);
    mesh->material = scene.CreateMaterial<EmissiveMaterial>(BE1::Vec3(43, 43, 35));
    mesh->verts[0].xyz = BE1::Vec3(-2.0f, -2.0f, 0);
    mesh->verts[1].xyz = BE1::Vec3(-2.0f, 2.0f, 0);
    mesh->verts[2].xyz = BE1::Vec3(2.0f, -2.0f, 0);
    mesh->verts[3].xyz = BE1::Vec3(2.0f, 2.0f, 0);

    mesh->indexes[0] = 0;
    mesh->indexes[1] = 1;
    mesh->indexes[2] = 2;
    mesh->indexes[3] = 2;
    mesh->indexes[4] = 1;
    mesh->indexes[5] = 3;

    lightModel->FinishMeshes();
    lightModel->BuildBVH();

    //scene.AddDirectionalLight(BE1::Vec3(0.2f, 0.2f, 1.0f), BE1::Vec3(1.0f, 1.0f, 1.0f));
    scene.AddModel(BE1::Vec3(0, 0, 0), BE1::Angles(0, 0, 0), objModel);
    scene.AddModel(BE1::Vec3(0, -8, 22), BE1::Angles(0, 0, 0), lightModel);

    BE1::Image *image = LoadTextureImage("lightprobes/glacier.hdr");
    auto lightProbeTexture = std::make_unique<TextureLightProbe>(image);
    scene.SetLightProbeTexture(std::move(lightProbeTexture));
}

void SceneBuilder::BuildSibenik() {
    SetUnitMeter(2.0f);
    camera.SetFocalDistance(10.f);
    camera.SetLensRadius(0.01f * GetUnitMeter());

    camera.Init(DEG2RAD(80), (float)screenHeight / screenWidth);

    camera.SetOrigin(BE1::Vec3(0.0f, 4.0f, -12.1f));
    camera.SetAngles(BE1::Angles(90.0f, 0.0f, 0.0f));

    Model *objModel = scene.CreateModel();
    objModel->LoadFromObj("models/sibenik/sibenik.obj");
    objModel->BuildBVH();

    Material *materials[5];
    materials[0] = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.4f, 0.4f, 0.4f), 0.4f);
    materials[1] = scene.CreateMaterial<PlasticMaterial>(BE1::Vec3(0.5f), BE1::Vec3(1.0f), 1.0f / 300.0f);
    materials[2] = scene.CreateMaterial<MetalMaterial>(BE1::Vec3(0.9f, 0.9f, 0.9f), BE1::Vec3(1.5f, 0.8f, 0.4f), 0.005f);
    materials[3] = scene.CreateMaterial<MirrorMaterial>(BE1::Vec3(0.5f, 0.5f, 0.5f));
    materials[4] = scene.CreateMaterial<GlassMaterial>(BE1::Vec3(0.9f, 0.9f, 0.9f), BE1::Vec3(0.9f, 0.9f, 0.9f), 2.5f);

    Model *lightModel = scene.CreateModel();
    SubMesh *mesh = lightModel->AllocSubMesh(4, 6);
    mesh->material = scene.CreateMaterial<EmissiveMaterial>(BE1::Vec3(43, 43, 35));
    mesh->verts[0].xyz = BE1::Vec3(-1.8f, -1.8f, 0);
    mesh->verts[1].xyz = BE1::Vec3(-1.8f, 1.8f, 0);
    mesh->verts[2].xyz = BE1::Vec3(1.8f, -1.8f, 0);
    mesh->verts[3].xyz = BE1::Vec3(1.8f, 1.8f, 0);

    mesh->indexes[0] = 0;
    mesh->indexes[1] = 1;
    mesh->indexes[2] = 2;
    mesh->indexes[3] = 2;
    mesh->indexes[4] = 1;
    mesh->indexes[5] = 3;

    lightModel->FinishMeshes();
    lightModel->BuildBVH();

    //scene.AddDirectionalLight(BE1::Vec3(0.2f, 0.2f, 1.0f), BE1::Vec3(1.0f, 1.0f, 1.0f));
    //scene.AddPointLight(BE1::Vec3(0, -10, 1), 12.0f, BE1::Vec3(1, 1, 1));
    scene.AddModel(BE1::Vec3(0, 0, 0), BE1::Angles(0, 0, 0), objModel);
    scene.AddModel(BE1::Vec3(0, -7.6f, 6), BE1::Angles(0, 0, 0), lightModel);

    BE1::Image *image = LoadTextureImage("lightprobes/doge2.hdr");
    auto lightProbeTexture = std::make_unique<TextureLightProbe>(image);
    scene.SetLightProbeTexture(std::move(lightProbeTexture));
}

void SceneBuilder::BuildOptimus() {
    SetUnitMeter(10.0f);
    camera.SetFocalDistance(10.f);
    camera.SetLensRadius(0.01f * GetUnitMeter());

    camera.Init(DEG2RAD(60), (float)screenHeight / screenWidth);
    camera.SetOrigin(BE1::Vec3(1.8f, 742.0f, 205.0f));
    camera.SetAngles(BE1::Angles(180.0f, 90.0f, 0.0f));

    Model *objModel = scene.CreateModel();
    //objModel->LoadFromObj("models/cornell_box.obj");
    objModel->LoadFromObj("models/optimus/T000.obj");
    objModel->BuildBVH();

    scene.AddModel(BE1::Vec3(0, 0, 0), BE1::Angles(0, 0, 0), objModel);
    //scene.AddDirectionalLight(BE1::Vec3(0.2f, 0.2f, 1.0f), BE1::Vec3(1.0f, 1.0f, 1.0f));

    BE1::Image *image = LoadTextureImage("lightprobes/glacier.hdr");
    auto lightProbeTexture = std::make_unique<TextureLightProbe>(image);
    scene.SetLightProbeTexture(std::move(lightProbeTexture));
}

#if 0
void SceneBuilder::BuildSponza() {
    SetUnitMeter(50.0f);
    camera.SetFocalDistance(20.f);
    camera.SetLensRadius(0.01f * GetUnitMeter());

    camera.Init(DEG2RAD(80), (float)screenHeight / screenWidth);
    camera.SetOrigin(BE1::Vec3(21.0f, 0.0f, 10.0f));
    camera.SetYawPitchRoll(180.0f, -10.0f, 0.0f);

    materialL = scene.CreateMaterial<EmissiveMaterial>(BE1::Vec3(15, 15, 15));

    SubMesh *objModel = new SubMesh;
    objModel->LoadFromObj("models/troll.obj");
    objModel->BuildBVH();

    SubMesh *lightModel = new SubMesh;
    lightModel->AllocSubMesh(4, 6);
    lightModel->verts[0].xyz = BE1::Vec3(-4.0f, -4.0f, 0);
    lightModel->verts[1].xyz = BE1::Vec3(-4.0f, 4.0f, 0);
    lightModel->verts[2].xyz = BE1::Vec3(4.0f, -4.0f, 0);
    lightModel->verts[3].xyz = BE1::Vec3(4.0f, 4.0f, 0);

    lightModel->indexes[0] = 0;
    lightModel->indexes[1] = 1;
    lightModel->indexes[2] = 2;
    lightModel->indexes[3] = 2;
    lightModel->indexes[4] = 1;
    lightModel->indexes[5] = 3;

    lightModel->FinishMesh();
    lightModel->BuildBVH();

    Material *materials[5];
    materials[0] = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(1.0f, 1.0f, 1.0f), 0.0f);
    materials[1] = scene.CreateMaterial<PlasticMaterial>(BE1::Vec3(0.4f, 0.02f, 0.04f), BE1::Vec3(1.0f), 1.0f / 180.0f);
    materials[2] = scene.CreateMaterial<PlasticMaterial>(BE1::Vec3(0.02f, 0.4f, 0.04f), BE1::Vec3(1.0f), 1.0f / 180.0f);
    materials[3] = scene.CreateMaterial<PlasticMaterial>(BE1::Vec3(0.04f, 0.1f, 0.5f), BE1::Vec3(1.0f), 1.0f / 180.0f);
    materials[4] = new cMetal(BE1::Vec3(0.998f, 0.998f, 0.998f), BE1::Vec3(1.5f, 1.5f, 1.5f), 0.001f);

    //scene.AddDirectionalLight(BE1::Vec3(1.0f, -1.0f, 3.5f), BE1::Vec3(0.9f, 0.9f, 1.0f));
    scene.AddPlane(Plane(BE1::Vec3(0, 0, 1), 0), materials[0]);
    scene.AddMesh(BE1::Vec3(10, 5, 0), BE1::Vec3(-30, 0, 0), objModel, materials[1]);
    scene.AddMesh(BE1::Vec3(0, -5, 0), BE1::Vec3(140, 0, 0), objModel, materials[2]);
    scene.AddMesh(BE1::Vec3(-10, -15, 0), BE1::Vec3(220, 0, 0), objModel, materials[3]);
    /*scene.AddSphere(Sphere(BE1::Vec3(-0.6f, 0, 0.6f), 0.3f), materials[0]);
    scene.AddSphere(Sphere(BE1::Vec3(0.6f, 0, 0.6f), 0.3f), materials[1]);
    scene.AddSphere(Sphere(BE1::Vec3(0.0f, 0, 0), 0.3f), materials[2]);
    scene.AddSphere(Sphere(BE1::Vec3(-0.6f, 0, -0.6f), 0.3f), materials[3]);
    scene.AddSphere(Sphere(BE1::Vec3(0.6f, 0, -0.6f), 0.3f), materials[4]);*/
    scene.AddMesh(BE1::Vec3(20, -10, 30), BE1::Vec3(0, 0, 0), lightModel, materialL);

    Image *image = new Image;
    //image->LoadHDR("lightprobes/pisa.hdr");
    //image->LoadHDR("lightprobes/uffizi-large.hdr");
    image->LoadHDR("lightprobes/glacier.hdr");
    //image->LoadHDR("lightprobes/grace-new.hdr");
    auto lightProbeTexture = std::make_unique<TextureLightProbe>(image);
    //scene.SetLightProbeTexture(std::move(lightProbeTexture));
}

void SceneBuilder::BuildOptimus() {
    SetUnitMeter(2000.0f);
    camera.SetFocalDistance(500.f);
    camera.SetLensRadius(0.01f * GetUnitMeter());

    camera.Init(DEG2RAD(45), (float)screenHeight / screenWidth);
    camera.SetOrigin(BE1::Vec3(0.0f, 550.0f, 50.0f));
    camera.SetYawPitchRoll(90.0f, 0.0f, 0.0f);

    SubMesh *objModel = new SubMesh;
    objModel->LoadFromObj("models/figure.obj");
    objModel->BuildBVH();

    Material *materials[5];
    materials[0] = scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.4f, 0.4f, 0.4f), 0.4f);
    materials[1] = scene.CreateMaterial<PlasticMaterial>(BE1::Vec3(0.5f), BE1::Vec3(1.0f), 1.0f / 300.0f);
    materials[2] = new cMetal(BE1::Vec3(0.9f, 0.9f, 0.9f), BE1::Vec3(1.5f, 0.8f, 0.4f), 0.005f);
    materials[3] = new cMirror(BE1::Vec3(0.5f, 0.5f, 0.5f));
    materials[4] = scene.CreateMaterial<GlassMaterial>(BE1::Vec3(0.9f, 0.9f, 0.9f), BE1::Vec3(0.9f, 0.9f, 0.9f), 2.5f);

    //scene.AddDirectionalLight(BE1::Vec3(1.0f, 0.0f, 0.0f), BE1::Vec3(1.0f, 0.0f, 0.5f));
    //scene.AddPlane(Plane(BE1::Vec3(0, 0, 1), 0), materials[0]);
    scene.AddMesh(BE1::Vec3(0, 0, 0), BE1::Vec3(90, 60, -90), objModel, materials[2]);

    Image *image = new Image;
    //image->LoadHDR("lightprobes/pisa.hdr");
    //image->LoadHDR("lightprobes/ennis.hdr");
    image->LoadHDR("lightprobes/uffizi-large.hdr");
    //image->LoadHDR("lightprobes/glacier.hdr");
    auto lightProbeTexture = std::make_unique<TextureLightProbe>(image);
    scene.SetLightProbeTexture(std::move(lightProbeTexture));
}
#endif

void SceneBuilder::BuildBuiltin() {
    SetUnitMeter(1.0f);
    camera.Init(DEG2RAD(55.0f), float(screenHeight) / screenWidth);
    camera.SetOrigin(BE1::Vec3(7.0f, 0.0f, 3.0f));
    camera.SetAngles(BE1::Angles(0.0f, 14.0f, 180.0f));
    camera.SetFocalDistance(7.0f);
    camera.SetLensRadius(0.025f);

    scene.AddPlane(BE1::Plane(BE1::Vec3(0, 0, 1), 0),
                   scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.48f, 0.50f, 0.54f), 0.0f));
    scene.AddSphere(BE1::Sphere(BE1::Vec3(0, -1.45f, 0.8f), 0.8f),
                    scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.65f, 0.08f, 0.035f), 0.25f));
    scene.AddSphere(BE1::Sphere(BE1::Vec3(-0.2f, 0.35f, 1.0f), 1.0f),
                    scene.CreateMaterial<MetalMaterial>(BE1::Vec3(0.95f), BE1::Vec3(1.8f, 1.324f, 0.432f), 0.025f));
    scene.AddSphere(BE1::Sphere(BE1::Vec3(0.7f, 1.9f, 0.65f), 0.65f),
                    scene.CreateMaterial<GlassMaterial>(BE1::Vec3(0.95f), BE1::Vec3(0.95f), 1.5f));

    // A downward-facing area light also exercises triangle BVH traversal.
    auto *lightModel = scene.CreateModel();
    auto *mesh = lightModel->AllocSubMesh(4, 6);
    mesh->material = scene.CreateMaterial<EmissiveMaterial>(BE1::Vec3(7.0f, 6.5f, 5.5f));
    mesh->verts[0].xyz = BE1::Vec3(-2, -2, 5);
    mesh->verts[1].xyz = BE1::Vec3(-2, 2, 5);
    mesh->verts[2].xyz = BE1::Vec3(2, -2, 5);
    mesh->verts[3].xyz = BE1::Vec3(2, 2, 5);
    const int indices[] = {0, 1, 2, 2, 1, 3};
    memcpy(mesh->indexes, indices, sizeof(indices));
    lightModel->FinishMeshes();
    lightModel->BuildBVH();
    scene.AddModel(BE1::Vec3::zero, BE1::Angles(0, 0, 0), lightModel);
    scene.AddDirectionalLight(BE1::Vec3(1, -0.6f, 2), BE1::Vec3(0.45f));
}

void SceneBuilder::BuildSkull() {
    Model *skullModel = scene.CreateModel();
    skullModel->LoadFromObj(SkullModelPath);
    if (skullModel->NumMeshes() == 0) {
        BE_LOG("TestTracer: could not load skull model; using built-in scene.\n");
        BuildBuiltin();
        return;
    }
    skullModel->BuildBVH();

    // This OBJ is Z-up; compensate for the loader's Y-up conversion and face +X.
    const BE1::Angles modelAngles(0.0f, 90.0f, 180.0f);
    BE1::AABB bounds;
    bounds.SetFromTransformedAABB(skullModel->GetAABB(), BE1::Vec3::zero, modelAngles.ToMat3());
    const BE1::Vec3 center = bounds.Center();
    const BE1::Vec3 position(-center.x, -center.y, -bounds.b[0].z);
    scene.AddModel(position, modelAngles, skullModel);

    const float height = bounds.b[1].z - bounds.b[0].z;
    SetUnitMeter(height);
    const BE1::Vec3 target(0.0f, 0.0f, height * 0.5f);
    const float fov = DEG2RAD(45.0f);
    const float aspectRatio = float(screenHeight) / screenWidth;
    // Fit the bounding sphere into the narrower field of view, with some margin.
    const float halfFov = atanf(tanf(fov * 0.5f) * BE1::Min(1.0f, aspectRatio));
    const float distance = bounds.Extents().Length() * 1.1f / sinf(halfFov);
    const BE1::Vec3 cameraOffset = BE1::Vec3(1.0f, -0.4f, 0.2f).Normalized() * distance;
    camera.Init(fov, aspectRatio);
    camera.SetOrigin(target + cameraOffset);
    camera.SetAngles((-cameraOffset).ToAngles());
    camera.SetFocalDistance(distance);
    camera.SetLensRadius(height * 0.001f);

    scene.AddPlane(BE1::Plane(BE1::Vec3(0, 0, 1), 0), scene.CreateMaterial<MatteMaterial>(BE1::Vec3(0.22f, 0.24f, 0.27f), 0.0f));

    Model *lightModel = scene.CreateModel();
    SubMesh *mesh = lightModel->AllocSubMesh(4, 6);
    mesh->material = scene.CreateMaterial<EmissiveMaterial>(BE1::Vec3(5.0f, 4.7f, 4.2f));
    const float halfSize = height * 0.6f;
    mesh->verts[0].xyz = BE1::Vec3(-halfSize, -halfSize, 0);
    mesh->verts[1].xyz = BE1::Vec3(-halfSize, halfSize, 0);
    mesh->verts[2].xyz = BE1::Vec3(halfSize, -halfSize, 0);
    mesh->verts[3].xyz = BE1::Vec3(halfSize, halfSize, 0);
    const int indices[] = {0, 1, 2, 2, 1, 3};
    memcpy(mesh->indexes, indices, sizeof(indices));
    lightModel->FinishMeshes();
    lightModel->BuildBVH();
    scene.AddModel(BE1::Vec3(height, -height * 0.6f, height * 1.8f), BE1::Angles(0, 0, 0), lightModel);
    scene.AddDirectionalLight(BE1::Vec3(1.0f, 0.6f, 0.5f), BE1::Vec3(0.2f, 0.24f, 0.3f));
}

void BuildScene(SceneManager &scene, SceneView &view, int width, int height, ScenePreset preset) {
    scene.Init();
    if (preset == ScenePreset::Default) {
        // Skull OBJ를 우선 사용하고, 없으면 기존 OBJ/HDR 또는 내장 장면을 사용한다.
        if (BE1::fileSystem.FileExists(SkullModelPath)) {
            preset = ScenePreset::Skull;
        } else if (BE1::fileSystem.FileExists("Data/models/killeroo.obj") &&
            BE1::fileSystem.FileExists("Data/lightprobes/pisa.hdr")) {
            preset = ScenePreset::Killeroo;
        } else {
            BE_LOG("TestTracer: original scene assets missing; using built-in scene.\n");
            preset = ScenePreset::Builtin;
        }
    }
    SceneBuilder builder(scene, view, width, height);
    switch (preset) {
    case ScenePreset::Skull:
        builder.BuildSkull();
        break;
    case ScenePreset::Killeroo:
        builder.BuildKilleroo();
        break;
    case ScenePreset::LegoCar:
        builder.BuildLegoCar();
        break;
    case ScenePreset::CornellBox:
        builder.BuildCornellBox();
        break;
    case ScenePreset::Sponza:
        builder.BuildSponza();
        break;
    case ScenePreset::Sibenik:
        builder.BuildSibenik();
        break;
    case ScenePreset::Optimus:
        builder.BuildOptimus();
        break;
    default:
        builder.BuildBuiltin();
        break;
    }
}
