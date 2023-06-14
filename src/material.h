// \brief
//		material
//

#pragma once

#include "pbrt.h"
#include "geometry.h"
#include "shape.h"
#include "bsdf.h"
#include "sampler.h"


namespace pbrt
{

// Material
class FMaterial
{
public:
	virtual ~FMaterial() {}

	virtual std::unique_ptr<FBSDF> Scattering(const FIntersection& isect, FSampler* sampler) const = 0;
};

// matte material
class FMatteMaterial : public FMaterial
{
public:
	FMatteMaterial(const FColor& diffuseColor)
		: diffuseColor(diffuseColor)
	{}

	std::unique_ptr<FBSDF> Scattering(const FIntersection& isect, FSampler* sampler) const override
	{
		return std::make_unique<FLambertionReflection>(FFrame(isect.normal), diffuseColor);
	}

protected:
	FColor diffuseColor;
};


// mirror material
class FMirrorMaterial : public FMaterial
{
public:
	FMirrorMaterial(const FColor& specularColor)
		: specularColor(specularColor)
	{}

	std::unique_ptr<FBSDF> Scattering(const FIntersection& isect, FSampler* sampler) const override
	{
		return std::make_unique<FSpecularReflection>(FFrame(isect.normal), specularColor);
	}

protected:
	FColor specularColor;
};


// glass material
class FGlassMaterial : public FMaterial
{
public:
	FGlassMaterial(Float eta, const FColor& reflection = FColor(1,1,1), const FColor& transmission=FColor(1,1,1))
		: eta(eta)
		, Kr(reflection)
		, Kt(transmission)
	{}

	std::unique_ptr<FBSDF> Scattering(const FIntersection& isect, FSampler* sampler) const override
	{
		return std::make_unique<FFresnelSpecular>(FFrame(isect.normal), (Float)1, eta, Kr, Kt);
	}

protected:
	Float eta;
	FColor Kr;  // Kr
	FColor Kt;  // Kt
};


// plastic material
class FPlasticMaterial : public FMaterial
{
public:
	FPlasticMaterial(const FColor& Kd, const FColor& Ks, Float roughness, bool remapRoughness)
		: Kd(Kd)
		, Ks(Ks)
		, roughness(roughness)
		, remapRoughness(remapRoughness)
	{
		Float Ld = Kd.Luminance();
		Float Ls = Ks.Luminance();
		Float L = Ld + Ls;
	
		Qd = Ld / L;
	}

	std::unique_ptr<FBSDF> Scattering(const FIntersection& isect, FSampler* sampler) const override;

protected:
	FColor Kd;
	FColor Ks;
	Float roughness;
	const bool remapRoughness;

	Float Qd;
};

// metal
class FMetalMaterial : public FMaterial
{
public:
	FMetalMaterial(const FColor& eta,
		const FColor& k,
		Float uRoughness,
		Float vRoughness,
		bool remapRoughness)
		: eta(eta)
		, k(k)
		, uRoughness(uRoughness)
		, vRoughness(vRoughness)
		, remapRoughness(remapRoughness)
	{
	}

	std::unique_ptr<FBSDF> Scattering(const FIntersection& isect, FSampler* sampler) const override;

protected:
	const FColor eta;
	const FColor k;
	Float uRoughness;
	Float vRoughness;
	bool remapRoughness;
};



} // namespace pbrt
