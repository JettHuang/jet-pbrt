// \brief
//		bsdf.cc
//

#include "bsdf.h"
#include "microfacet.h"


namespace pbrt
{

//////////////////////////////////////////////////////////////////////////
// Fresnel

Fresnel::~Fresnel() {}
FColor FresnelConductor::Evaluate(Float cosThetaI) const 
{
	return fresnel_conductor(std::abs(cosThetaI), etaI, etaT, k);
}

FColor FresnelDielectric::Evaluate(Float cosThetaI) const 
{
	return fresnel_dielectric(cosThetaI, etaI, etaT);
}

//////////////////////////////////////////////////////////////////////////
// Micro facet model

FMicrofacetReflection::~FMicrofacetReflection()
{
	delete distribution;
	delete fresnel;
}

FColor FMicrofacetReflection::Evalf_Local(const FVector3& wo, const FVector3& wi) const
{
	Float cosThetaO = abs_cos_theta(wo), cosThetaI = abs_cos_theta(wi);
	FVector3 wh = wi + wo;

	// Handle degenerate cases for microfacet reflection
	if (cosThetaI == 0 || cosThetaO == 0) return FColor(0);
	if (wh.x == 0 && wh.y == 0 && wh.z == 0) return FColor(0);

	wh = Normalize(wh);
	// For the Fresnel call, make sure that wh is in the same hemisphere
	// as the surface normal, so that TIR is handled correctly.
	FColor F = fresnel->Evaluate(Dot(wi, face_forward(wh, FVector3(0, 0, 1))));
	return R * distribution->D(wh) * distribution->G(wo, wi) * F /
		(4 * cosThetaI * cosThetaO);
}

Float FMicrofacetReflection::Pdf_Local(const FVector3& wo, const FVector3& wi) const
{
	if (!same_hemisphere(wo, wi)) return 0;
	FVector3 wh = Normalize(wo + wi);
	return distribution->Pdf(wo, wh) / (4 * Dot(wo, wh));
}

FBSDFSample FMicrofacetReflection::Sample_Local(const FVector3& wo, const FFloat2& random) const
{
	// Sample microfacet orientation $\wh$ and reflected direction $\wi$
	FBSDFSample sample;
	if (wo.z == 0) return sample;

	FVector3 wh = distribution->Sample_wh(wo, random);
	if (Dot(wo, wh) < 0) return sample;

	FVector3 wi = reflect(wo, wh);
	if (!same_hemisphere(wo, wi)) return sample;

	// Compute PDF of _wi_ for microfacet reflection
	sample.wi = wi;
	sample.f = Evalf_Local(wo, wi);
	sample.pdf = distribution->Pdf(wo, wh) / (4 * Dot(wo, wh));
	sample.ebsdf = eBSDFType::Reflection | eBSDFType::Glossy;

	return sample;
}

FMicrofacetTransmission::~FMicrofacetTransmission()
{
	delete distribution;
}

FColor FMicrofacetTransmission::Evalf_Local(const FVector3& wo, const FVector3& wi) const
{
	if (same_hemisphere(wo, wi)) return 0;  // transmission only

	Float cosThetaO = cos_theta(wo);
	Float cosThetaI = cos_theta(wi);
	if (cosThetaI == 0 || cosThetaO == 0) return FColor(0);

	// Compute $\wh$ from $\wo$ and $\wi$ for microfacet transmission
	Float eta = cos_theta(wo) > 0 ? (etaB / etaA) : (etaA / etaB);
	FVector3 wh = Normalize(wo + wi * eta);
	if (wh.z < 0) wh = -wh;

	// Same side?
	if (Dot(wo, wh) * Dot(wi, wh) > 0) return FColor(0);

	FColor F = fresnel.Evaluate(Dot(wo, wh));

	Float sqrtDenom = Dot(wo, wh) + eta * Dot(wi, wh);
	Float factor = (1 / eta);

	return (FColor(1) - F) * T *
		std::abs(distribution->D(wh) * distribution->G(wo, wi) * eta * eta *
			AbsDot(wi, wh) * AbsDot(wo, wh) * factor * factor /
			(cosThetaI * cosThetaO * sqrtDenom * sqrtDenom));
}

Float FMicrofacetTransmission::Pdf_Local(const FVector3& wo, const FVector3& wi) const
{
	if (same_hemisphere(wo, wi)) return 0;
	// Compute $\wh$ from $\wo$ and $\wi$ for microfacet transmission
	Float eta = cos_theta(wo) > 0 ? (etaB / etaA) : (etaA / etaB);
	FVector3 wh = Normalize(wo + wi * eta);

	if (Dot(wo, wh) * Dot(wi, wh) > 0) return 0;

	// Compute change of variables _dwh\_dwi_ for microfacet transmission
	Float sqrtDenom = Dot(wo, wh) + eta * Dot(wi, wh);
	Float dwh_dwi =
		std::abs((eta * eta * Dot(wi, wh)) / (sqrtDenom * sqrtDenom));
	return distribution->Pdf(wo, wh) * dwh_dwi;
}

FBSDFSample FMicrofacetTransmission::Sample_Local(const FVector3& wo, const FFloat2& random) const
{
	FBSDFSample sample;

	if (wo.z == 0) return sample;
	FVector3 wh = distribution->Sample_wh(wo, random);
	if (Dot(wo, wh) < 0) return sample;  // Should be rare

	FVector3 wi;
	Float eta = cos_theta(wo) > 0 ? (etaA / etaB) : (etaB / etaA);
	if (!refract(wo, wh, eta, &wi)) return sample;
	
	sample.wi = wi;
	sample.pdf= Pdf(wo, wi);
	sample.f = Evalf_Local(wo, wi);
	sample.ebsdf = eBSDFType::Transmission | eBSDFType::Glossy;
	return sample;
}



} // namespace pbrt
