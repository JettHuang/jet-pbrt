// \brief
//		integrator.cc
//

#include "integrator.h"


namespace pbrt
{

void FIntegrator::Render(const FScene* scene, FSampler* sampler, FFilm* film)
{
	const FCamera* pCamera = scene->Camera();
	const FVector2 filmsize = film->GetResolution();
	int width = (int)filmsize.x;
	int height = (int)filmsize.y;

	Float ratio = (Float)1 / sampler->GetSamplesPerPixel();
	for (int y = 0; y < height; y++)
	{
		std::cerr << "\rScanlines remaining: " << (height - y) << ' ' << std::flush;
		for (int x = 0; x <width; x++)
		{
			FColor L;

			sampler->StartPixel();

			do 
			{
				auto camera_sample = sampler->GetCameraSample(FPoint2((Float)x, (Float)y));
				FRay ray = pCamera->GenerateRay(camera_sample);

				FColor dL = Li(ray, scene, sampler) * ratio;

				DOCHECK(dL.IsValid());
				L += dL;
			} while (sampler->NextSample());

			film->AddColor(x, y, Clamp01(L));
		} // end x
	} // end y

}

//////////////////////////////////////////////////////////////////////////
// Whitted Integrator
FColor FWhittedIntegrator::Li(const FRay& ray, const FScene* scene, FSampler* sampler, int depth)
{
	FColor L(0,0,0);

	// Find closest ray intersection or return background radiance
	FIntersection isect;
	if (!scene->Intersect(ray, isect))
	{
		for (const auto& light : scene->lights)
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
	for (const auto& light : scene->lights)
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


FColor FWhittedIntegrator::SpecularReflect(const FRay& ray, const FIntersection& isect, const FBSDF* bsdfptr, const FScene* scene, FSampler* sampler, int depth)
{
	const int matchFlags = eBSDFType::Specular | eBSDFType::Reflection;

	FBSDFSample bsdfsample = bsdfptr->Sample(isect.wo, sampler->GetFloat2());
	if ((bsdfsample.ebsdf & matchFlags) != matchFlags)
	{
		return FColor::Black;
	}

	return bsdfsample.f * Li(isect.SpawnRay(bsdfsample.wi), scene, sampler, depth + 1) * AbsDot(bsdfsample.wi, isect.normal) / bsdfsample.pdf;
}

FColor FWhittedIntegrator::SpecularTransmit(const FRay& ray, const FIntersection& isect, const FBSDF* bsdfptr, const FScene* scene, FSampler* sampler, int depth)
{
	const int matchFlags = eBSDFType::Specular | eBSDFType::Transmission;

	FBSDFSample bsdfsample = bsdfptr->Sample(isect.wo, sampler->GetFloat2());
	if ((bsdfsample.ebsdf & matchFlags) != matchFlags)
	{
		return FColor::Black;
	}

	return bsdfsample.f * Li(isect.SpawnRay(bsdfsample.wi), scene, sampler, depth + 1) * AbsDot(bsdfsample.wi, isect.normal) / bsdfsample.pdf;
}


} // namespace pbrt
