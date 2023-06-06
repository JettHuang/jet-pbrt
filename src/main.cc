// \brief
//		main.cc
//

#include "pbrt.h"
#include "light.h"
#include "integrator.h"


using namespace pbrt;


std::shared_ptr<FScene> create_cornellbox_box_scene(const FVector2& filmsize)
{
	const FPoint3 lookfrom(278, 273, -800);
	const FPoint3 lookat(278, 273, 0);
	const FVector3 vup(0, 1, 0);
	const Float vfov = 60.0;

	std::shared_ptr<FScene> scene = std::make_shared<FScene>();

	scene->CreateCamera<FCamera>(lookfrom, Normalize(lookat - lookfrom), vup, vfov, filmsize);

	const FColor backgroundclr(0.70f, 0.80f, 1.00f);
	scene->CreateLight<FEnvironmentLight>(FPoint3(0, 0, 0), 1, backgroundclr);

	std::shared_ptr<FMaterial> red = scene->CreateMaterial<FMatteMaterial>(FColor(0.63f, 0.065f, 0.05f));
	std::shared_ptr<FMaterial> green = scene->CreateMaterial<FMatteMaterial>(FColor(0.14f, 0.45f, 0.091f));
	std::shared_ptr<FMaterial> white = scene->CreateMaterial<FMatteMaterial>(FColor(0.725f, 0.71f, 0.68f));
	
	// light
	//std::shared_ptr<FMaterial> lightmat = scene->CreateMaterial<FMatteMaterial>(FColor(0.65f, 0.65f, 0.65f));
	//std::shared_ptr<FTriangleMesh> area_light0 = FTriangleMesh::LoadTriangleMesh("scene\\cornellbox\\light.obj", lightmat.get());
	//scene->AddPrimitive(area_light0);
	const FColor intensity(8.0f * FVector3(0.747f + 0.058f, 0.747f + 0.258f, 0.747f) + 15.6f * FVector3(0.740f + 0.287f, 0.740f + 0.160f, 0.740f) + 18.4f * FVector3(0.737f + 0.642f, 0.737f + 0.159f, 0.737f));
	scene->CreateLight<FPointLight>(FVector3(278, 273, 0), 1, FColor(0.63f, 0.065f, 0.05f));

	// wall
	std::shared_ptr<FTriangleMesh> floor = FTriangleMesh::LoadTriangleMesh("scene\\cornellbox\\floor.obj", white.get());
	scene->AddPrimitive(floor);
	std::shared_ptr<FTriangleMesh> shortbox = FTriangleMesh::LoadTriangleMesh("scene\\cornellbox\\shortbox.obj", white.get());
	scene->AddPrimitive(shortbox);
	std::shared_ptr<FTriangleMesh> tallbox = FTriangleMesh::LoadTriangleMesh("scene\\cornellbox\\tallbox.obj", white.get());
	scene->AddPrimitive(tallbox);
	std::shared_ptr<FTriangleMesh> left = FTriangleMesh::LoadTriangleMesh("scene\\cornellbox\\left.obj", red.get());
	scene->AddPrimitive(left);
	std::shared_ptr<FTriangleMesh> right = FTriangleMesh::LoadTriangleMesh("scene\\cornellbox\\right.obj", green.get());
	scene->AddPrimitive(right);

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
	integrator.Render(scene.get(), sampler.get(), &film);

	film.SaveAsImage("cornellbox_debug", EImageType::BMP);

	return 0;
}
