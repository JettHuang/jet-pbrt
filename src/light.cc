// \brief
//		light.cc
//

#include "light.h"
#include "scene.h"


namespace pbrt
{

void FLight::Preprocess(const FScene& scene)
{
		// do nothing
}

void FDirectionLight::Preprocess(const FScene& scene)
{
	FBounds3 bound = scene.WorldBound();

	bound.BoundingSphere(worldCenter, worldRadius);
	area = kPi * worldRadius * worldRadius;
	power = irradiance * area;
}

void FEnvironmentLight::Preprocess(const FScene& scene)
{
	FBounds3 bound = scene.WorldBound();

	bound.BoundingSphere(worldCenter, worldRadius);
	area = kPi * worldRadius * worldRadius;
	power = radiance * area;
}


} // namespace pbrt
