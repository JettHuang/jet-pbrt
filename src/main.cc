// \brief
//		main.cc
//

#include "pbrt.h"
#include "light.h"
#include "integrator.h"


using namespace pbrt;


std::shared_ptr<FScene> create_cornellbox_box_scene(const FVector2& filmsize)
{
	const FPoint3 lookfrom(278, 273, 960);
	const FPoint3 lookat(278, 273, 0);
	const FVector3 vup(0, 1, 0);
	const Float vfov = 60.0;

	std::shared_ptr<FScene> scene = std::make_shared<FScene>();

	scene->CreateCamera<FCamera>(lookfrom, Normalize(lookat - lookfrom), vup, vfov, filmsize);

	const FColor backgroundclr(0.0f, 0.0f, 0.00f);
	scene->CreateLight<FEnvironmentLight>(FPoint3(0, 0, 0), 1, backgroundclr);

	std::shared_ptr<FMaterial> red = scene->CreateMaterial<FMatteMaterial>(FColor(0.63f, 0.065f, 0.05f));
	std::shared_ptr<FMaterial> green = scene->CreateMaterial<FMatteMaterial>(FColor(0.14f, 0.45f, 0.091f));
	std::shared_ptr<FMaterial> white = scene->CreateMaterial<FMatteMaterial>(FColor(0.725f, 0.71f, 0.68f));
	
	// light
	std::shared_ptr<FMaterial> mat_light0 = scene->CreateMaterial<FMatteMaterial>(FColor(0.65f, 0.65f, 0.65f));
	std::vector<std::shared_ptr<FShape>> shape_light0 = scene->CreateTriangleMesh("scene\\cornellbox\\light.obj", true, true);
	const FColor radiance(8.0f * FVector3(0.747f + 0.058f, 0.747f + 0.258f, 0.747f) + 15.6f * FVector3(0.740f + 0.287f, 0.740f + 0.160f, 0.740f) + 18.4f * FVector3(0.737f + 0.642f, 0.737f + 0.159f, 0.737f));
	scene->CreateAreaLights(1, radiance, shape_light0, mat_light0);
	
	//scene->CreateLight<FPointLight>(FVector3(278, 273, 0), 1, FColor(0.63f, 0.065f, 0.05f));

	// wall
	std::vector<std::shared_ptr<FShape>> floor = scene->CreateTriangleMesh("scene\\cornellbox\\floor.obj", true, true);
	scene->CreatePrimitives(floor, white);

	std::vector<std::shared_ptr<FShape>> shortbox = scene->CreateTriangleMesh("scene\\cornellbox\\shortbox.obj", true, true);
	scene->CreatePrimitives(shortbox, white);

	std::vector<std::shared_ptr<FShape>> tallbox = scene->CreateTriangleMesh("scene\\cornellbox\\tallbox.obj", true, true);
	scene->CreatePrimitives(tallbox, white);

	std::vector<std::shared_ptr<FShape>> left = scene->CreateTriangleMesh("scene\\cornellbox\\left.obj", true, true);
	scene->CreatePrimitives(left, red);

	std::vector<std::shared_ptr<FShape>> right = scene->CreateTriangleMesh("scene\\cornellbox\\right.obj", true, true);
	scene->CreatePrimitives(right, green);

	scene->Preprocess();
	return scene;
}

int main(int argc, char* agrv[])
{
	const int width = 600, height = 600;
	FFilm film(width, height);

	std::shared_ptr<FScene> scene = create_cornellbox_box_scene(film.GetResolution());

	int samples_per_pixel = 50;
	std::shared_ptr<FSampler> sampler = std::make_shared<FRandomSampler>(samples_per_pixel);

	FWhittedIntegrator integrator(5);
	//FDebugIntegrator integrator;
	integrator.Render(scene.get(), sampler.get(), &film);

	film.SaveAsImage("cornellbox_debug", EImageType::BMP);

	return 0;
}
