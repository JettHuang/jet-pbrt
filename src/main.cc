// \brief
//		main.cc
//

#include "pbrt.h"
#include "integrator.h"


using namespace pbrt;

int main(int argc, char* agrv[])
{
	const int width = 600, height = 600;
	FFilm film(width, height);

	std::shared_ptr<FScene> scene = create_random_scene(film.GetResolution());

	int samples_per_pixel = 50;
	std::shared_ptr<FSampler> sampler = std::make_shared<FRandomSampler>(samples_per_pixel);

	FWhittedIntegrator integrator(5);
	integrator.Render(scene.get(), sampler.get(), &film);

	film.SaveAsImage("cornellbox_debug", EImageType::BMP);

	return 0;
}
