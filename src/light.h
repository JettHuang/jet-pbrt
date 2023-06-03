// \brief
//		lights
//

#pragma once

#include "pbrt.h"
#include "geometry.h"
#include "color.h"
#include "shape.h"


namespace pbrt
{

enum eLightType
{
	PositionLight = 1,   // delta light
	DirectionLight = 2,  // delta light

	AreaLight = 4,
	InfiniteLight = 8
};

inline bool is_delta_light(int lightFlags)
{
	return lightFlags & (eLightType::PositionLight | eLightType::DirectionLight);
}

// light sample
struct FLightSample
{
	FPoint3		pos;
	FVector3	wi;
	Float		pdf;
	FColor		Li;

	FLightSample() : pdf(0) {}
	FLightSample(const FIntersection& isect, const FVector3& wi, Float pdf, const FColor& li)
		: pos(isect.position)
		, wi(wi)
		, pdf(pdf)
		, Li(li)
	{}
};

class FScene;

// Light
class FLight
{
public:
	virtual ~FLight() { }

	FLight(const FPoint3& worldpos, int samplesNum=1)
		: worldPosition(worldpos)
		, samplesNum(samplesNum)
	{}

	virtual bool IsDelta() const = 0;
	virtual bool IsFinite() const = 0;

	virtual void Preprocess(const FScene& scene);
	virtual FColor Power() const = 0;
	
	// only work for environment light
	virtual FColor Le(const FRay& r) const { return FColor::Black; }

	virtual FLightSample Sample_Li(const FIntersection& isect, const FFloat2& random) const = 0;
	virtual Float Pdf_Li(const FIntersection& isect, const FVector3& world_wi) const = 0;

protected:
	FPoint3	worldPosition;
	int		samplesNum;
};

// Point Light
class FPointLight : public FLight
{
public:
	FPointLight(const FPoint3& worldpos, int samplesNum, const FColor& intensity)
		: FLight(worldpos, samplesNum)
		, intensity(intensity)
	{}

	bool IsDelta() const override { return true; }
	bool IsFinite() const override { return true; }

	FColor Power() const override { return intensity * 4 * kPi; }

	FLightSample Sample_Li(const FIntersection& isect, const FFloat2& random) const override
	{
		FLightSample sample;
		sample.pos = worldPosition;
		sample.wi = Normalize(worldPosition - isect.position);
		sample.pdf = (Float)1;

		/*
		  if a sphere hold the point light, then:
			  $$ \Phi= A E = \frac{A I} {l^{2}} $$, where $$ A = 4 \pi r^{2} $$

		  or from the definition:
			  $$ \mathrm{d} \Phi= \mathrm{d} A E = \frac{\mathrm{d} A I} {l^{2}} $$

		  finally we have:
			 $$ E = \frac{I}{l^{2}}$$


		  as for `Lo = f * Li * cos_theta / pdf`,

		  for area light:
			  Lo = f * Li * cos_theta / pdf = f * (Li * cos_theta * projected_solid_angle), see `shape::pdf_direction(...)`

		  for point light:
			  Lo = f * Li * cos_theta / pdf = f * (E * cos_theta / 1), it's no problem
		*/
		sample.Li = intensity / Distance2(worldPosition, isect.position);

		return sample;
	}

	Float Pdf_Li(const FIntersection& isect, const FVector3& world_wi) const override
	{
		return (Float)0;
	}

protected:
	FColor	intensity;
};


// Direction light,  use a disk to simulate
class FDirectionLight : public FLight
{
public:
	FDirectionLight(const FPoint3& worldpos, int samplesNum, const FColor& irradiance, const FVector3& worlddir)
		: FLight(worldpos, samplesNum)
		, irradiance(irradiance)
		, worldDir(Normalize(worlddir))
		, frame(worlddir)
	{
		area = worldRadius = (Float)0;
	}

	bool IsDelta() const override { return true; }
	bool IsFinite() const override { return false; }

	void Preprocess(const FScene& scene) override;
	FColor Power() const override { return power; }


	FLightSample Sample_Li(const FIntersection& isect, const FFloat2& random) const override
	{
		FLightSample sample;
		sample.wi = -worldDir;
		sample.pos = isect.position + sample.wi * 2 * worldRadius;
		sample.pdf = (Float)1;
		sample.Li = irradiance;

		return sample;
	}

	Float Pdf_Li(const FIntersection& isect, const FVector3& world_wi) const override
	{
		return (Float)0;
	}

protected:
	FColor	irradiance;
	FPoint3	worldCenter;
	Float	worldRadius;
	Float	area;
	FColor	power;

	FVector3 worldDir;
	FFrame	frame;
};

// Area Light
class FAreaLight : public FLight
{
public:
	FAreaLight(const FPoint3& worldpos, int samplesNum, const FColor& radiance, const FShape* inShape)
		: FLight(worldpos, samplesNum)
		, radiance(radiance)
		, shape(inShape)
	{
		power = radiance * shape->Area() * kPi;
	}

	bool IsDelta() const override { return false; }
	bool IsFinite() const override { return true; }

	FColor Power() const override { return power; }

	FLightSample Sample_Li(const FIntersection& isect, const FFloat2& random) const override
	{
		FLightSample sample;
		FLightIntersection light_isect = shape->SampleDirection(isect, random, &sample.pdf);
		sample.pos = light_isect.position;

		if (sample.pdf == 0 || (light_isect.position - isect.position).Length2() == 0)
		{
			sample.Li = FColor::Black;
		}
		else
		{
			sample.wi = Normalize(light_isect.position - isect.position);
			sample.Li = Le(light_isect, -sample.wi);
		}

		return sample;
	}

	Float Pdf_Li(const FIntersection& isect, const FVector3& world_wi) const override
	{
		return shape->Pdf_Direction(isect, world_wi);
	}

	/*
	   isect
	   ----
		 ^    ^
		  \   |
		wo \  | normal
			\ |
			 \|
		   -------
		 light_isect
	*/
	FColor Le(const FLightIntersection& light_isect, const FVector3& wo) const
	{
		return (Dot(light_isect.normal, wo) > 0) ? radiance : FColor::Black;
	}

protected:
	FColor	radiance;
	FColor	power;
	const FShape *shape;
};

// constant_environment_light_t
// use a sphere hold all the scene to simulate
class FEnvironmentLight : public FLight
{
public:
	FEnvironmentLight(const FPoint3& worldpos, int samplesNum, const FColor& radiance)
		: FLight(worldpos, samplesNum)
		, radiance(radiance)
		, worldRadius(0)
		, area(0)
	{
	}

	bool IsDelta() const override { return false; }
	bool IsFinite() const override { return false; }

	void Preprocess(const FScene& scene) override;
	FColor Power() const override { return power; }

	FLightSample Sample_Li(const FIntersection& isect, const FFloat2& random) const override
	{
		FLightSample sample;
		sample.wi = uniform_sphere_sample(random);
		sample.pos = isect.position + sample.wi * 2 * worldRadius;

		Float theta = SphericalTheta(sample.wi);
		Float sin_theta = std::sin(theta);
		sample.pdf = 1 / (2 * kPi * kPi * sin_theta);
		if (sin_theta == 0)
			sample.pdf = 0;

		sample.Li = radiance;

		return sample;
	}

	Float Pdf_Li(const FIntersection& isect, const FVector3& world_wi) const override
	{
		Float theta = SphericalTheta(world_wi);
		Float sin_theta = std::sin(theta);
		
		if (sin_theta == 0)
			return 0;

		return 1 / (2 * kPi * kPi * sin_theta);
	}

	FColor Le(const FRay& ray) const override
	{
		return radiance;
	}

protected:
	FColor radiance;
	FPoint3	worldCenter;
	Float worldRadius;
	Float area;
	FColor power;
};

} // namespace pbrt

