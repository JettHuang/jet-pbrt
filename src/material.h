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

	virtual std::unique_ptr<FBSDF> Scattering(const FIntersection& isect) const = 0;
};

// matte material
class FMatteMaterial : public FMaterial
{
public:
	FMatteMaterial(const FColor& diffuseColor)
		: diffuseColor(diffuseColor)
	{}

	std::unique_ptr<FBSDF> Scattering(const FIntersection& isect) const override
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

	std::unique_ptr<FBSDF> Scattering(const FIntersection& isect) const override
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

	std::unique_ptr<FBSDF> Scattering(const FIntersection& isect) const override
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
	FPlasticMaterial(const FColor& diffuse, const FColor& specular, Float shininess)
		: diffuseColor(diffuse)
		, specularColor(specular)
		, exponent(shininess)
		, diffuse_probility(1)
		, specular_probility(0)
	{
		Float diffuselum = diffuseColor.Luminance();
		Float specularlum = specularColor.Luminance();
		Float luminance = diffuselum + specularlum;
	
		diffuse_probility = diffuselum / luminance;
		specular_probility = specularlum / luminance;
	}

	std::unique_ptr<FBSDF> Scattering(const FIntersection& isect) const override
	{
		float_t random = rng.uniform_float();
		if (random < specular_probility)
		{
			return std::make_unique<FPhongSpecularReflection>(FFrame(isect.normal), specularColor / specular_probility, exponent);
		}
		else
		{
			return std::make_unique<FLambertionReflection>(FFrame(isect.normal), diffuseColor / diffuse_probility);
		}
	}

protected:
	FColor diffuseColor;
	FColor specularColor;
	Float exponent;

	Float diffuse_probility;
	Float specular_probility;
	
	mutable FRNG rng;
};

} // namespace pbrt
