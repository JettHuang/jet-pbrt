// \brief
//		material.cc
//

#include "material.h"
#include "microfacet.h"


namespace pbrt
{

std::unique_ptr<FBSDF> FPlasticMaterial::Scattering(const FIntersection& isect, FSampler* sampler) const
{
	Float u = sampler->GetFloat();
	if (u < Qd)
	{
		return std::make_unique<FLambertionReflection>(FFrame(isect.normal), Kd / Qd);
	}
	else
	{
		Fresnel* fresnel = new FresnelDielectric(1.5f, 1.f);
		// Create microfacet distribution _distrib_ for plastic material
		Float rough = roughness;
		if (remapRoughness)
			rough = TrowbridgeReitzDistribution::RoughnessToAlpha(rough);
		MicrofacetDistribution* distrib = new TrowbridgeReitzDistribution(rough, rough);
		return std::make_unique<FMicrofacetReflection>(FFrame(isect.normal), Ks / (1 - Qd), distrib, fresnel);
	}
}

std::unique_ptr<FBSDF> FMetalMaterial::Scattering(const FIntersection& isect, FSampler* sampler) const
{
	Float uRough = uRoughness;
	Float vRough = vRoughness;
	if (remapRoughness) {
		uRough = TrowbridgeReitzDistribution::RoughnessToAlpha(uRough);
		vRough = TrowbridgeReitzDistribution::RoughnessToAlpha(vRough);
	}
	Fresnel* frMf = new FresnelConductor(1.f, eta, k);
	MicrofacetDistribution* distrib = new TrowbridgeReitzDistribution(uRough, vRough);
	
	return std::make_unique<FMicrofacetReflection>(FFrame(isect.normal), FColor(1), distrib, frMf);
}



} // namespace pbrt
