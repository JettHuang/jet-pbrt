// \brief
//		integrator.cc
//

#include "integrator.h"
#include "parallel.h"


namespace pbrt
{

// Task
class FRenderTask : public FTask
{
public:
	FRenderTask(const FIntegrator* inIntegrator, const FScene* inScene, const std::shared_ptr<FSampler>& inSampler, const FFilmView &inFilmView)
		: integrator(inIntegrator)
		, scene(inScene)
		, sampler(inSampler)
		, filmview(inFilmView)
	{}

	virtual void Execute() override
	{
		integrator->DoRender(scene, sampler.get(), &filmview);
	}

protected:
	const FIntegrator* integrator;
	const FScene* scene;
	std::shared_ptr<FSampler> sampler;
	FFilmView filmview;
};

void FIntegrator::Render(const FScene* scene, FSampler* sampler, FFilm* film, int numthreads) const
{
	const FVector2 resolution = film->GetResolution();
	const int width = (int)resolution.x;
	const int height = (int)resolution.y;

	FPerformanceCounter perf;
	perf.StartPerf();

	PBRT_PRINT("start rendering ...\n");
	if (numthreads < 1)
	{
		FFilmView filmview(film, 0, 0, width, height);

		DoRender(scene, sampler, &filmview);
	}
	else
	{
		const int lines_per_task = 20;
		FParallelSystem  parallel;
		std::vector<std::shared_ptr<FRenderTask>>  tasks;
		
		for (int y=0;; y+=lines_per_task)
		{
			if (y >= height) {
				break;
			}

			int endy = y + lines_per_task;
			if (endy > height) { endy = height; }

			std::shared_ptr<FSampler> dupsampler = sampler->Clone();
			std::shared_ptr<FRenderTask> task = std::make_shared<FRenderTask>(this, scene, dupsampler, FFilmView(film, 0, y, width, endy));
			tasks.push_back(task);

			parallel.AddTask(task.get());
		} // end for 

		parallel.Start(numthreads);
		parallel.WaitForFinish();
	}

	double elapse = perf.EndPerf();
	PBRT_PRINT("finish rendering ...\n");
	PBRT_PRINT("FIntegrator::Render used %f seconds.\n", (float)(elapse / 1000000.0));
}

void FIntegrator::DoRender(const FScene* scene, FSampler* sampler, FFilmView* filmview) const
{
	const FCamera* pCamera = scene->Camera();
	int startx, starty, endx, endy;

	filmview->GetViewport(startx, starty, endx, endy);

	Float ratio = (Float)1 / sampler->GetSamplesPerPixel();
	for (int y = starty; y < endy; y++)
	{
		for (int x = startx; x < endx; x++)
		{
			FColor L;

			sampler->StartPixel();

			do
			{
				auto camera_sample = sampler->GetCameraSample(FPoint2((Float)x, (Float)y));
				FRay ray = pCamera->GenerateRay(camera_sample);

				FColor dL = Li(ray, scene, sampler) * ratio;

				PBRT_DOCHECK(dL.IsValid());
				L += dL;
			} while (sampler->NextSample());

			filmview->AddColor(x, y, Clamp01(L));
		} // end x
	} // end y
}

//////////////////////////////////////////////////////////////////////////
// Whitted Integrator
FColor FWhittedIntegrator::Li(const FRay& ray, const FScene* scene, FSampler* sampler, int depth) const
{
	FColor L(0,0,0);

	// Find closest ray intersection or return background radiance
	FIntersection isect;
	bool bHit = scene->Intersect(ray, isect);
	if (!bHit)
	{
		for (const auto& light : scene->InfiniteLights())
			L += light->Le(ray);

		return L;
	}

	// Compute emitted and reflected light at ray intersection point
	// Initialize common variables for Whitted integrator
	const FNormal3& N = isect.normal;

	// Compute scattering function for surface interaction
	std::unique_ptr<FBSDF> bsdfptr = isect.Bsdf();
	if (!bsdfptr) {
		return Li(isect.SpawnRay(ray.Dir()), scene, sampler, depth);
	}

	// Compute emitted light if ray hit an area light source
	L += isect.Le();

	// Add contribution of each light source
	for (const auto& light : scene->Lights())
	{
		FLightSample lightsample = light->Sample_Li(isect, sampler->GetFloat2());
		if (lightsample.Li.IsBlack() || lightsample.pdf == (Float)0) {
			continue;
		}

		FColor f = bsdfptr->Evalf(isect.wo, lightsample.wi);
		if (!f.IsBlack() && !scene->Occluded(isect, lightsample.pos))
		{
			L += f * lightsample.Li * AbsDot(lightsample.wi, N) / lightsample.pdf;
		}
	} // end for 

	// reflect & transmit
	if (depth + 1 < maxDepth)
	{
		// Trace rays for specular reflection and refraction
		L += SpecularReflect(ray, isect, bsdfptr.get(), scene, sampler, depth);
		L += SpecularTransmit(ray, isect, bsdfptr.get(), scene, sampler, depth);
	}

	return L;
}


FColor FWhittedIntegrator::SpecularReflect(const FRay& ray, const FIntersection& isect, const FBSDF* bsdfptr, const FScene* scene, FSampler* sampler, int depth) const
{
	const int matchFlags = eBSDFType::Specular | eBSDFType::Reflection;

	FBSDFSample bsdfsample = bsdfptr->Sample(isect.wo, sampler->GetFloat2());
	if ((bsdfsample.ebsdf & matchFlags) != matchFlags)
	{
		return FColor::Black;
	}

	return bsdfsample.f * Li(isect.SpawnRay(bsdfsample.wi), scene, sampler, depth + 1) * AbsDot(bsdfsample.wi, isect.normal) / bsdfsample.pdf;
}

FColor FWhittedIntegrator::SpecularTransmit(const FRay& ray, const FIntersection& isect, const FBSDF* bsdfptr, const FScene* scene, FSampler* sampler, int depth) const
{
	const int matchFlags = eBSDFType::Specular | eBSDFType::Transmission;

	FBSDFSample bsdfsample = bsdfptr->Sample(isect.wo, sampler->GetFloat2());
	if ((bsdfsample.ebsdf & matchFlags) != matchFlags)
	{
		return FColor::Black;
	}

	return bsdfsample.f * Li(isect.SpawnRay(bsdfsample.wi), scene, sampler, depth + 1) * AbsDot(bsdfsample.wi, isect.normal) / bsdfsample.pdf;
}

//////////////////////////////////////////////////////////////////////////
// Path Integrator Recursive
// Li = Lo = Le + ¡ÒLi
//         = Le + ¡Ò(Le + ¡ÒLi)
//         = Le + ¡ÒLe + ¡Ò(¡ÒLi)
//         = Le + ¡ÒLe + ¡Ò(¡Ò(Le + ¡ÒLi))
//         = Le + ¡ÒLe + ¡Ò(¡ÒLe + ¡Ò(¡ÒLi))
//         = Le + ¡ÒLe + ¡Ò(¡ÒLe + ¡Ò(¡Ò(Le + ¡ÒLi)))
//         = Le + ¡ÒLe + ¡Ò(¡ÒLe + ¡Ò(¡ÒLe + ¡Ò(¡ÒLi)))
//         = Le + ¡ÒLe + ¡Ò(¡ÒLe + ¡Ò(¡ÒLe + ¡Ò(¡ÒLe + ...))) < --LOOK THIS

FColor FPathIntegratorRecursive::Li(const FRay& ray, const FScene* scene, FSampler* sampler, int depth, bool is_prev_specular) const
{
	FColor L(0, 0, 0);

	// Find closest ray intersection or return background radiance
	FIntersection isect;
	bool bFoundIntersection = scene->Intersect(ray, isect);
	if (depth == 0 || is_prev_specular)
	{
		if (bFoundIntersection) {
			L += isect.Le();
		}
		else {
			for (const auto& light : scene->InfiniteLights())
				L += light->Le(ray);
		}
	}
	
	// Terminate path if ray escaped or _maxDepth_ was reached
	if (!bFoundIntersection || depth >= maxDepth)
	{
		return L;
	}

	const FNormal3& N = isect.normal;

	// Compute scattering function for surface interaction
	std::unique_ptr<FBSDF> bsdfptr = isect.Bsdf();
	if (!bsdfptr) {
		return Li(isect.SpawnRay(ray.Dir()), scene, sampler, depth, is_prev_specular);
	}

	// Sample illumination from lights to find path contribution.
	// (But skip this for perfectly specular BSDFs.)
	if (!bsdfptr->IsDelta())
	{
		for (const auto& light : scene->Lights())
		{
			FLightSample lightsample = light->Sample_Li(isect, sampler->GetFloat2());
			if (lightsample.Li.IsBlack() || lightsample.pdf == (Float)0) {
				continue;
			}

			FColor f = bsdfptr->Evalf(isect.wo, lightsample.wi);
			if (!f.IsBlack() && !scene->Occluded(isect, lightsample.pos))
			{
				L += f * lightsample.Li * AbsDot(lightsample.wi, N) / lightsample.pdf;
			}
		} // end for 
	}
	
	// Sample BSDF to get new path direction
	FBSDFSample bsdfsample = bsdfptr->Sample(isect.wo, sampler->GetFloat2());
	if (bsdfsample.f.IsBlack() || bsdfsample.pdf == 0.f)
	{
		return L;
	}

	// Possibly terminate the path with Russian roulette.
	if (depth >= 3)
	{
		Float q = std::max((Float)0.05, 1 - bsdfsample.f.MaxComponentValue());
		if (sampler->GetFloat() < q)
		{
			return L;
		}

		L += bsdfsample.f * AbsDot(bsdfsample.wi, isect.normal) * Li(isect.SpawnRay(bsdfsample.wi), scene, sampler, depth + 1, bsdfptr->IsDelta()) / (bsdfsample.pdf * (1 - q));
		return L;
	}

	// for first 3 paths.
	L += bsdfsample.f * AbsDot(bsdfsample.wi, isect.normal) * Li(isect.SpawnRay(bsdfsample.wi), scene, sampler, depth + 1, bsdfptr->IsDelta()) / bsdfsample.pdf;
	return L;
}

//////////////////////////////////////////////////////////////////////////
// Path Integrator iteration
//
//  iteration style path tracing
//  Li = Le + T*Le + T*(T*Le + T*(T*Le + ...))
//	   = Le + T*Le + T^2*Le  + ...
//
FColor FPathIntegratorIteration::Li(const FRay& inRay, const FScene* scene, FSampler* sampler) const
{
	FColor L(0, 0, 0),  beta(1, 1, 1);
	FRay ray(inRay);
	bool bSpecularBounce = false;
	int bounces;

	for (bounces = 0; ; ++bounces)
	{
		// Find closest ray intersection or return background radiance
		FIntersection isect;
		bool bFoundIntersection = scene->Intersect(ray, isect);
		if (bounces == 0 || bSpecularBounce)
		{
			if (bFoundIntersection) {
				L += beta * isect.Le();
			}
			else {
				for (const auto& light : scene->InfiniteLights())
					L += beta * light->Le(ray);
			}
		}

		// Terminate path if ray escaped or _maxDepth_ was reached
		if (!bFoundIntersection || bounces >= maxDepth)
		{
			break;
		}

		const FNormal3& N = isect.normal;

		// Compute scattering function for surface interaction
		std::unique_ptr<FBSDF> bsdfptr = isect.Bsdf();
		if (!bsdfptr) {
			ray = isect.SpawnRay(ray.Dir());
			--bounces;
			continue;
		}

		// Sample illumination from lights to find path contribution.
		// (But skip this for perfectly specular BSDFs.)
		if (!bsdfptr->IsDelta())
		{
			for (const auto& light : scene->Lights())
			{
				FLightSample lightsample = light->Sample_Li(isect, sampler->GetFloat2());
				if (lightsample.Li.IsBlack() || lightsample.pdf == (Float)0) {
					continue;
				}

				FColor f = bsdfptr->Evalf(isect.wo, lightsample.wi);
				if (!f.IsBlack() && !scene->Occluded(isect, lightsample.pos))
				{
					L += beta * f * lightsample.Li * AbsDot(lightsample.wi, N) / lightsample.pdf;
				}
			} // end for 
		}

		// Sample BSDF to get new path direction
		FBSDFSample bsdfsample = bsdfptr->Sample(isect.wo, sampler->GetFloat2());
		if (bsdfsample.f.IsBlack() || bsdfsample.pdf == 0.f)
		{
			break;
		}

		bSpecularBounce = bsdfsample.ebsdf & eBSDFType::Specular;
		// Possibly terminate the path with Russian roulette.
		if (bounces >= 3)
		{
			Float q = std::max((Float)0.05, 1 - bsdfsample.f.MaxComponentValue());
			if (sampler->GetFloat() < q)
			{
				break;
			}

			beta *= bsdfsample.f * AbsDot(bsdfsample.wi, isect.normal) / (bsdfsample.pdf * (1 - q));
			ray = isect.SpawnRay(bsdfsample.wi);
		}
		else
		{
			// for first 3 paths.
			beta *= bsdfsample.f * AbsDot(bsdfsample.wi, isect.normal) / bsdfsample.pdf;
			ray = isect.SpawnRay(bsdfsample.wi);
		}
	} // end for 

	return L;
}

} // namespace pbrt
