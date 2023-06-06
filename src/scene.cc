// \brief
//		scene.cc
//

#include "scene.h"


namespace pbrt
{

void FScene::Preprocess()
{
	CalculateWorldBound();

	for (std::shared_ptr<FLight>& light : lights)
	{
		light->Preprocess(*this);
	} // end for 

	// build bvh
	bvh = std::make_shared<FBVH_Node<FPrimitive*>>(shadow_primitives, 0, shadow_primitives.size());
	shadow_bvh = bvh.get();
}

bool FScene::Intersect(const FRay& ray, FIntersection& oisect) const
{
#if 0
	bool bHit = false;

	for (auto primitive : shadow_primitives)
	{
		if (primitive->Intersect(ray, isect))
		{
			bHit = true;
		}
	} // end for i

	return bHit;
#else
	return shadow_bvh->Intersect(ray, oisect);
#endif
}

void FScene::CalculateWorldBound()
{
	FBounds3 bound;

    for (auto primitive : primitives)
	{
        bound.Expand(primitive->WorldBounds());
	}

	worldBound = bound;
}

//////////////////////////////////////////////////////////////////////////
// add interfaces
void FScene::AddPrimitive(const std::shared_ptr<FPrimitive>& inPrimitive)
{
	primitives.push_back(inPrimitive);
	shadow_primitives.push_back(inPrimitive.get());
}

//////////////////////////////////////////////////////////////////////////
// cornell box scene

std::shared_ptr<FScene> create_cornellbox_scene(const FVector2& filmsize)
{
	const FPoint3 lookfrom(278, 278, -2000);
	const FPoint3 lookat(278, 278, 0);
	const FVector3 vup(0, 1, 0);
	const Float vfov = 40.0f;

	std::shared_ptr<FScene> scene = std::make_shared<FScene>();

	scene->camera = std::make_shared<FCamera>(
		lookfrom,
		Normalize(lookat - lookfrom),
		vup,
		vfov, filmsize);
   
	const FColor backgroundclr(0.0f, 0.0f, 0.0f);
	scene->environment_light = std::make_shared<FEnvironmentLight>(FPoint3(0, 0, 0), 1, backgroundclr);;
	scene->lights.push_back(scene->environment_light);

	// materials
	std::shared_ptr<FMatteMaterial> red_mat = std::make_shared<FMatteMaterial>(FColor(.65f, .05f, .05f));
	std::shared_ptr<FMatteMaterial> white_mat = std::make_shared<FMatteMaterial>(FColor(.73f, .73f, .73f));
	std::shared_ptr<FMatteMaterial> green_mat = std::make_shared<FMatteMaterial>(FColor(.12f, .45f, .15f));

	scene->materials.push_back(red_mat);
	scene->materials.push_back(white_mat);
	scene->materials.push_back(green_mat);

	// primitives
	std::shared_ptr<FShape> shape_left = std::make_shared<FRectangle>(FRectangle::FromYZ(0, 555, 0, 555, 0, false));
	std::shared_ptr<FPrimitive> primitive_left = std::make_shared<FPrimitive>(shape_left.get(), green_mat.get(), nullptr);

	std::shared_ptr<FShape> shape_right = std::make_shared<FRectangle>(FRectangle::FromYZ(0, 555, 0, 555, 555, true));
	std::shared_ptr<FPrimitive> primitive_right = std::make_shared<FPrimitive>(shape_right.get(), red_mat.get(), nullptr);

	std::shared_ptr<FShape> shape_top = std::make_shared<FRectangle>(FRectangle::FromXZ(0, 555, 0, 555, 555 , true));
	std::shared_ptr<FPrimitive> primitive_top = std::make_shared<FPrimitive>(shape_top.get(), white_mat.get(), nullptr);

	std::shared_ptr<FShape> shape_bot = std::make_shared<FRectangle>(FRectangle::FromXZ(0, 555, 0, 555, 0, false));
	std::shared_ptr<FPrimitive> primitive_bot = std::make_shared<FPrimitive>(shape_bot.get(), white_mat.get(), nullptr);
	
	std::shared_ptr<FShape> shape_back = std::make_shared<FRectangle>(FRectangle::FromXY(0, 555, 0, 555, 555, true));
	std::shared_ptr<FPrimitive> primitive_back = std::make_shared<FPrimitive>(shape_back.get(), white_mat.get(), nullptr);

	std::shared_ptr<FShape> shape_light = std::make_shared<FRectangle>(FRectangle::FromXZ(213, 343, 227, 332, 554, true));
	std::shared_ptr<FAreaLight> area_light0 = std::make_shared<FAreaLight>(FVector3(0,0,0), 1, FColor(15,15,15), shape_light.get());
	std::shared_ptr<FPrimitive> primitive_light = std::make_shared<FPrimitive>(shape_light.get(), white_mat.get(), area_light0.get());

	// ball
	std::shared_ptr<FShape> shape_ball0 = std::make_shared<FSphere>(FPoint3(160, 100, 145), 100.f);
	std::shared_ptr<FMaterial> mat_ball0 = std::make_shared<FMatteMaterial>(FColor(0.1f, 0.8f, 0.f));
	std::shared_ptr<FPrimitive> primitive_ball0 = std::make_shared<FPrimitive>(shape_ball0.get(), mat_ball0.get(), nullptr);

	scene->lights.push_back(area_light0);
	scene->shapes.push_back(shape_left);
	scene->shapes.push_back(shape_right);
	scene->shapes.push_back(shape_top);
	scene->shapes.push_back(shape_bot);
	scene->shapes.push_back(shape_back);
	scene->shapes.push_back(shape_light);
	scene->shapes.push_back(shape_ball0);

#if 0
	scene->primitives.push_back(primitive_left);
	scene->primitives.push_back(primitive_right);
	scene->primitives.push_back(primitive_bot);
	scene->primitives.push_back(primitive_top);
	scene->primitives.push_back(primitive_back);
#endif
	scene->primitives.push_back(primitive_light);
	scene->primitives.push_back(primitive_ball0);

	scene->materials.push_back(mat_ball0);

	scene->Preprocess();
    return scene;
}

std::shared_ptr<FScene> create_random_scene(const FVector2& filmsize)
{
	const FPoint3 lookfrom(278, 273, -800);
	const FPoint3 lookat(100, 0, 0);
	const FVector3 vup(0, 1, 0);
	const Float vfov = 60.0;
	
	std::shared_ptr<FScene> scene = std::make_shared<FScene>();

	scene->CreateCamera<FCamera>(lookfrom, Normalize(lookat - lookfrom), vup, vfov, filmsize);

	const FColor backgroundclr(0.70f, 0.80f, 1.00f);
	scene->CreateLight<FEnvironmentLight>(FPoint3(0, 0, 0), 1, backgroundclr);

	std::shared_ptr<FMatteMaterial> groundmat = scene->CreateMaterial<FMatteMaterial>(FColor(0.2f, 0.3f, 0.1f));
	std::shared_ptr<FShape> groundshape = scene->CreateShape<FSphere>(FPoint3(0, -1000, 0), 1000.f);
	std::shared_ptr<FPrimitive> groundprimitive = scene->CreatePrimitive(groundshape.get(), groundmat.get(), nullptr);

	const int count = 5;
	for (int a = -count; a < count; a++) {
		for (int b = -count; b < count; b++) {
			auto choose_mat = random_double();
			FPoint3 center(a + 200.f * random_double(), 0.2f, b + 200.f * random_double());

			if ((center - FPoint3(4, 0.2f, 0)).Length() > 50.f) {
				std::shared_ptr<FMaterial> sphere_material;
				std::shared_ptr<FShape> sphere_shape;
				std::shared_ptr<FPrimitive> sphere_primitive;

				if (choose_mat < 0.8f) {
					// diffuse
					FColor albedo(random_double() * random_double(), random_double() * random_double(), random_double() * random_double());
					sphere_material = scene->CreateMaterial<FMatteMaterial>(albedo);
					sphere_shape = scene->CreateShape<FSphere>(center, 10.f);
					sphere_primitive = scene->CreatePrimitive(sphere_shape.get(), sphere_material.get(), nullptr);
				}
				else if (choose_mat < 0.95f) {
					// glass
					sphere_material = scene->CreateMaterial<FGlassMaterial>(1.6f);
					sphere_shape = scene->CreateShape<FSphere>(center, 20.f);
					sphere_primitive = scene->CreatePrimitive(sphere_shape.get(), sphere_material.get(), nullptr);
				}
				else {
					// plastic
					sphere_material = scene->CreateMaterial<FPlasticMaterial>(FColor(.1f, .1f, .1f), FColor(.7f, .7f, .7f), 90.f);
					sphere_shape = scene->CreateShape<FSphere>(center, 15.f);
					sphere_primitive = scene->CreatePrimitive(sphere_shape.get(), sphere_material.get(), nullptr);
				}
			}
		}
	}

	std::shared_ptr<FMaterial> box_material = scene->CreateMaterial<FGlassMaterial>(1.6f);
	std::shared_ptr<FTriangleMesh> box_trianglemesh = FTriangleMesh::LoadTriangleMesh("scene\\cornellbox\\shortbox.obj", box_material.get());
	scene->AddPrimitive(box_trianglemesh);

	scene->Preprocess();
	return scene;
}



} // namespace pbrt
