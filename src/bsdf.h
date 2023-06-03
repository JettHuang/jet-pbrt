// \brief
//		bsdf
//

#pragma once

#include "pbrt.h"
#include "geometry.h"
#include "sampling.h"


namespace pbrt
{

// the functions below are based on local shading coordinate

inline Float cos_theta(const FVector3& w) { return w.z; }
inline Float abs_cos_theta(const FVector3& w) { return std::abs(w.z); }

inline bool same_hemisphere(const FVector3& w, const FVector3& wp) { return w.z * wp.z > 0; }

inline FVector3 reflect(const FVector3& wo, const FNormal3& normal)
{
	// https://www.pbr-book.org/3ed-2018/Reflection_Models/Specular_Reflection_and_Transmission#SpecularReflection

	return -wo + 2 * Dot(wo, normal) * normal;
}

// eta = eta_i/eta_t
inline bool refract(const FVector3& wi, const FNormal3& normal, Float eta, FVector3* out_wt)
{
	// https://www.pbr-book.org/3ed-2018/Reflection_Models/Specular_Reflection_and_Transmission#SpecularTransmission
	// https://github.com/mmp/pbrt-v3/blob/master/src/core/reflection.h#L97-L109

	// compute $\cos \theta_\mathrm{t}$ using Snell's law
	Float cos_theta_i = Dot(normal, wi);
	Float sin_theta_i_sq = std::max(Float(0), Float(1 - cos_theta_i * cos_theta_i));
	Float sin_theta_t_sq = eta * eta * sin_theta_i_sq;

	if (sin_theta_t_sq >= 1)
		return false; // handle total internal reflection for transmission

	Float cos_theta_t = std::sqrt(1 - sin_theta_t_sq);
	*out_wt = eta * -wi + (eta * cos_theta_i - cos_theta_t) * normal;

	DOCHECK(out_wt->IsValid() && !out_wt->IsZero());
	return true;
}

// Fresnel
inline Float fresnel_dielectric(Float cos_theta_i, Float eta_i, Float eta_t)
{
	// https://www.pbr-book.org/3ed-2018/Reflection_Models/Specular_Reflection_and_Transmission#FresnelReflectance
	// https://github.com/infancy/pbrt-v3/blob/master/src/core/reflection.cpp#L66-L90

	cos_theta_i = Clamp(cos_theta_i, (Float)-1, (Float)1);

	bool entering = cos_theta_i > 0.f;
	if (!entering)
	{
		std::swap(eta_i, eta_t);
		cos_theta_i = std::abs(cos_theta_i);
	}


	// compute $\cos \theta_\mathrm{t}$ using Snell's law
	Float sin_theta_i = std::sqrt(std::max((Float)0, 1 - cos_theta_i * cos_theta_i));
	Float sin_theta_t = eta_i / eta_t * sin_theta_i;

	// Handle total internal reflection
	if (sin_theta_t >= 1)
		return 1;

	Float cos_theta_t = std::sqrt(std::max((Float)0, 1 - sin_theta_t * sin_theta_t));


	Float r_para = ((eta_t * cos_theta_i) - (eta_i * cos_theta_t)) /
		((eta_t * cos_theta_i) + (eta_i * cos_theta_t));
	Float r_perp = ((eta_i * cos_theta_i) - (eta_t * cos_theta_t)) /
		((eta_i * cos_theta_i) + (eta_t * cos_theta_t));
	return (r_para * r_para + r_perp * r_perp) / 2;
}

// schlick approximation 1994
inline Float fresnel_dielectric_schlick(
	Float cos_theta_i, Float cos_theta_t,
	Float eta_i, Float eta_t)
{
	/*
	cos_theta_i = std::clamp(cos_theta_i, -1.0, 1.0);
	cos_theta_t = std::clamp(cos_theta_t, -1.0, 1.0);

	bool entering = cos_theta_i > 0.f;
	if (!entering)
	{
		std::swap(eta_i, eta_t);
		cos_theta_i = std::abs(cos_theta_i);
		cos_theta_t = std::abs(cos_theta_t);
	}
	*/

	Float F0 = (eta_t - eta_i) / (eta_t + eta_i);
	F0 *= F0;

	//Float cos_i = eta_i < eta_t ? cos_theta_i : cos_theta_t;
	Float cos_i = cos_theta_i < 0 ? -cos_theta_i : cos_theta_t;

	return Lerp(F0, 1.0f, std::pow(1 - cos_i, 5.0f));
}

inline Float fresnel_dielectric_schlick(
	Float cos_theta_i,
	Float eta_i, Float eta_t)
{
	Float F0 = (eta_t - eta_i) / (eta_t + eta_i);
	F0 *= F0;

	return Lerp(F0, 1.0f, std::pow(1 - cos_theta_i, 5.0f));
}

/*
   given
	 * the cosine of the incidence angle `cos_theta_i`,
	 * the fresnel reflectance at normal incidence `F0`
   compute reflectance
*/
inline Float fresnel_dielectric_schlick(Float cos_theta_i, Float F0)
{
	return Lerp(F0, 1.0f, std::pow((1.0f - cos_theta_i), 5.0f));
}


//////////////////////////////////////////////////////////////////////////
/*
   reference:
	 * LuxCoreRender Materials https://wiki.luxcorerender.org/LuxCoreRender_Materials
	 * Shader ¡ª Blender Manual https://docs.blender.org/manual/en/latest/render/shader_nodes/shader/index.html
	 * BSDFs - Mitsuba 3 https://mitsuba.readthedocs.io/en/latest/src/generated/plugins_bsdfs.html
*/

enum eBSDFType
{
	None = 0,
	Reflection = 1,
	Transmission = 2,
	Scattering = Reflection | Transmission,

	// distribute
	Specular = 4,
	Diffuse = 8,
	Glossy = 16
};

inline bool is_delta_bsdf(int type)
{
	return type & eBSDFType::Specular;
}

/*
  local shading frame:

	  z/n(0, 0, 1)
	   |
	   |
	   |
	   |
	   |_ _ _ _ _ _ x/s(1, 0, 0)
	  / p
	 /
	/
  y/t(0, 1, 0)

  prev   n   light
  ----   ^   -----
	^    |    ^
	 \   | ¦È /
   wo \  |  / wi is unknown, sampling from bsdf or light
	   \ | /
		\|/
	  -------
	   isect

   https://www.pbr-book.org/3ed-2018/Reflection_Models#x0-GeometricSetting
*/
struct FBSDFSample
{
	FColor	f;		// BSDF function value
	FVector3 wi;	// world wi
	Float	 pdf;	// pdf of this sample
	int		 ebsdf; // bsdf flags

	FBSDFSample()
		: wi(0, 0, 1)
		, pdf(0)
		, ebsdf(0)
	{}
};

// BSDF class
class FBSDF
{
public:
	virtual ~FBSDF() {}

	FBSDF(const FFrame &frame) 
		: shading_frame(frame)
	{
	}

public:
	virtual bool IsDelta() const = 0;

	// or called `f()`, `evaluate()`
	FColor Evalf(const FVector3& world_wo, const FVector3& world_wi) const
	{
		return Evalf_Local(ToLocal(world_wo), ToLocal(world_wi));
	}

	Float Pdf(const FVector3& world_wo, const FVector3& world_wi) const
	{
		return Pdf_Local(ToLocal(world_wo), ToLocal(world_wi));
	}

	// or called `sample_f()`, `sample_direction()`, `sample_solid_angle()`
	FBSDFSample Sample(const FVector3& world_wo, const FFloat2& random) const
	{
		FBSDFSample sample = Sample_Local(ToLocal(world_wo), random);
		sample.wi = ToWorld(sample.wi); // <--- ATTENTION!!!

		return sample;
	}

	FColor Eval_And_Pdf(const FVector3& world_wo, const FVector3& world_wi, Float *pdf) const
	{
		FVector3 wo = ToLocal(world_wo), wi = ToLocal(world_wi);

		*pdf = Pdf_Local(wo, wi);
		return Evalf_Local(wo, wi);
	}

protected:
	virtual FColor Evalf_Local(const FVector3& wo, const FVector3& wi) const = 0;
	virtual Float Pdf_Local(const FVector3& wo, const FVector3& wi) const = 0;

	virtual FBSDFSample Sample_Local(const FVector3& wo, const FFloat2& random) const = 0;

private:
	FVector3 ToLocal(const FVector3& world_vec3) const
	{
		return shading_frame.ToLocal(world_vec3);
	}

	FVector3 ToWorld(const FVector3& local_vec3) const
	{
		return shading_frame.ToWorld(local_vec3);
	}

private:
	FFrame shading_frame;
};


// Lambertion
class FLambertionReflection : public FBSDF
{
public:
	FLambertionReflection(const FFrame& shading_frame, const FColor& albedo) 
		: FBSDF(shading_frame)
		, albedo(albedo)
	{
	}

	bool IsDelta() const override { return false; }

	FColor Evalf_Local(const FVector3& wo, const FVector3& wi) const override
	{
		// TODO: confirm
		if (!same_hemisphere(wo, wi))
			return FColor::Zero;

		// lambertion surface's albedo divided by $\pi$ is surface bidirectional reflectance
		return albedo * kInvPi;
	}

	Float Pdf_Local(const FVector3& wo, const FVector3& wi) const override
	{
		return same_hemisphere(wo, wi) ? cosine_hemisphere_pdf(abs_cos_theta(wi)) : 0;
	}

	FBSDFSample Sample_Local(const FVector3& wo, const FFloat2& random) const override
	{
		FBSDFSample sample;

		// cosine-sample the hemisphere, flipping the direction if necessary
		sample.wi = cosine_hemisphere_sample(random);
		if (wo.z < 0)
			sample.wi.z *= -1;

		sample.f = Evalf_Local(wo, sample.wi);
		sample.pdf = Pdf_Local(wo, sample.wi);
		sample.ebsdf = eBSDFType::Reflection | eBSDFType::Diffuse;

		DOCHECK(sample.f.IsValid());
		return sample;
	}

private:
	// https://wiki.luxcorerender.org/LuxCoreRender_Materials_Matte
	// https://mitsuba.readthedocs.io/en/latest/src/generated/plugins_bsdfs.html#smooth-diffuse-material-diffuse
	// surface directional-hemispherical reflectance, usually called `albedo`
	// symbol: $\rho_{\mathrm{hd}}$
	FColor albedo;
};

/*
  ideal specular reflection, ignore fresnel effect,
  only suitable for some metal materials

  as a delta bsdf, it's `eval(...), pdf(...) sample(...)` functions requires special processing,
  same to `fresnel_specular_t`
*/
class FSpecularReflection : public FBSDF
{
public:
	FSpecularReflection(const FFrame& frame, const FColor& reflectance) 
		: FBSDF(frame)
		, reflectance{ reflectance}
	{
	}

	bool IsDelta() const override { return true; }

	FColor Evalf_Local(const FVector3& wo, const FVector3& wi) const override
	{
		return FColor::Zero;
	}

	Float Pdf_Local(const FVector3& wo, const FVector3& wi) const override
	{
		return 0;
	}

	FBSDFSample Sample_Local(const FVector3& wo, const FFloat2& random) const override
	{
		// https://www.pbr-book.org/3ed-2018/Reflection_Models/Specular_Reflection_and_Transmission#SpecularReflection
		// https://github.com/infancy/pbrt-v3/blob/master/src/materials/mirror.cpp#L45-L57  mirror material use `FresnelNoOp`
		// https://github.com/infancy/pbrt-v3/blob/master/src/core/reflection.h#L387-L408   class SpecularReflection;
		// https://github.com/infancy/pbrt-v3/blob/master/src/core/reflection.cpp#L181-L191 SpecularReflection::Sample_f(...)

		FBSDFSample sample;
		sample.wi = FVector3(-wo.x, -wo.y, wo.z); // sample.wi = reflect(wo, vec3_t(0, 0, 1));
		sample.f = reflectance / abs_cos_theta(sample.wi); // (f / cos_theta) * Li * cos_theta / pdf => f * Li
		sample.pdf = 1;
		sample.ebsdf = eBSDFType::Reflection | eBSDFType::Specular;

		DOCHECK(sample.f.IsValid());
		return sample;
	}

private:
	// https://wiki.luxcorerender.org/LuxCoreRender_Materials_Mirror
	FColor reflectance;
};


/*
   https://www.pbr-book.org/3ed-2018/Reflection_Models/Specular%20transmission%20projections.svg

   ray            N
	*             |             *
	   *     ¦È_i  |          *
		  *       |       *
			 *    |    *            outside ior: eta_i
				* | *
	- - - - - - - - - - - - - - - - interface
				  |*
				  | *               inside ior:  eta_t
				  |  *
				  |   *
				  |    *
				  | ¦È_t *
*/
class FFresnelSpecular : public FBSDF
{
public:
	FFresnelSpecular(
		const FFrame& frame, Float eta_i, Float eta_t, const FColor& reflectance, const FColor& transmittance)
		: FBSDF(frame)
		, eta_i(eta_i)
		, eta_t(eta_t)
		, reflectance(reflectance)
		, transmittance(transmittance)
	{
	}

	bool IsDelta() const override { return true; }

	FColor Evalf_Local(const FVector3& wo, const FVector3& wi) const override
	{
		return FColor::Zero;
	}

	Float Pdf_Local(const FVector3& wo, const FVector3& wi) const override
	{
		return 0;
	}

	FBSDFSample Sample_Local(const FVector3& wo, const FFloat2& random) const override
	{
		// https://www.pbr-book.org/3ed-2018/Reflection_Models/Specular_Reflection_and_Transmission#FresnelReflectance
		// 
		// https://github.com/infancy/pbrt-v3/blob/master/src/materials/glass.cpp#L64-L69   full smooth glass
		// https://github.com/infancy/pbrt-v3/blob/master/src/core/reflection.h#L440-L463   class FresnelSpecular;
		// https://github.com/infancy/pbrt-v3/blob/master/src/core/reflection.cpp#L627-L667 FresnelSpecular::Sample_f(...)


		FBSDFSample sample;

		if (wo.z == (Float)0)
		{
			return sample;
		}

		// percentage of light's reflect and refract
		Float reflect_percent = fresnel_dielectric(cos_theta(wo), eta_i, eta_t);
		Float refract_percent = 1 - reflect_percent;

		// and probability of single ray is reflect or refract
		Float Pr_reflect = reflect_percent, Pr_refract = refract_percent;

		// Russian roulette
		if (random.x < Pr_reflect)
		{
			// specular reflection

			sample.wi = FVector3(-wo.x, -wo.y, wo.z);

			sample.pdf = Pr_reflect;
			sample.f = (reflectance * reflect_percent) / abs_cos_theta(sample.wi);
			sample.ebsdf = eBSDFType::Reflection | eBSDFType::Specular;

			DOCHECK(sample.f.IsValid());
		}
		else
		{
			// specular refract/transmission

			FNormal3 normal(0, 0, 1); // use `z` as normal
			bool into = normal.Dot(wo) > 0; // ray from outside going in?

			FNormal3 wo_normal = into ? normal : normal * -1;
			Float eta = into ? eta_i / eta_t : eta_t / eta_i;

			if (refract(wo, wo_normal, eta, &sample.wi))
			{
				sample.pdf = Pr_refract;
				sample.f = (transmittance * refract_percent) / abs_cos_theta(sample.wi);
				sample.ebsdf = eBSDFType::Transmission | eBSDFType::Specular;

				DOCHECK(sample.f.IsValid());
			}
			else
			{
				sample.f = FColor::Zero; // total internal reflection
			}
		}

		return sample;
	}

private:
	// outside and inside ior of interface
	Float eta_i; // ior_i_
	Float eta_t; // ior_t_

	// https://wiki.luxcorerender.org/LuxCoreRender_Materials_Glass
	// https://mitsuba.readthedocs.io/en/latest/src/generated/plugins_bsdfs.html#smooth-dielectric-material-dielectric
	// optional factor that can be used to modulate the specular reflection/transmission component. 
	FColor reflectance;
	FColor transmittance;
};


// physically based(energy conservation) Phong specular reflection model
// Lafortune and Willems, ¡°Using the modified Phong reflectance model for physically based rendering¡±, Technical Report http://graphics.cs.kuleuven.be/publications/Phong/
class FPhongSpecularReflection : public FBSDF
{
public:
	FPhongSpecularReflection(const FFrame& frame,  const FColor& Ks, Float exponent)
		: FBSDF(frame)
		, Ks(Ks)
		, exponent(exponent)
	{
	}

	bool IsDelta() const override { return false; }

	FColor Evalf_Local(const FVector3& wo, const FVector3& wi) const override
	{
		// TODO: confirm
		if (!same_hemisphere(wo, wi))
			return FColor::Zero;

		const FVector3 wr = reflect(wo, FVector3(0, 0, 1));
		const Float cos_alpha = Dot(wr, wi);

		const FColor rho = Ks * (exponent + 2.f) * kInv2Pi;
		return rho * std::pow(cos_alpha, exponent);
	}

	Float Pdf_Local(const FVector3& wo, const FVector3& wi) const override
	{
		const FVector3 wr = reflect(wo, FVector3(0, 0, 1));
		//const Float cos_alpha = dot(wr, wi);

		return cosine_hemisphere_pdf_phong(wr, wi);
	}

	FBSDFSample Sample_Local(const FVector3& wo, const FFloat2& random) const override
	{
		FBSDFSample sample;

		// TODO
		sample.wi = cosine_hemisphere_sample_phong(random);

		const FVector3 wr = reflect(wo, FVector3(0, 0, 1));
		FFrame frame(wr);
		sample.wi = frame.ToWorld(sample.wi);

		if (wo.z < 0)
			sample.wi.z *= -1;

		sample.f = Evalf_Local(wo, sample.wi);
		sample.pdf = Pdf_Local(wo, sample.wi);
		sample.ebsdf = eBSDFType::Reflection | eBSDFType::Glossy;

		return sample;
	}

private:
	// cosine lobe hemisphere sampling
	FVector3 cosine_hemisphere_sample_phong(const FVector2& random) const
	{
		const Float phi = 2 * kPi * random.x;
		const Float cos_theta = std::pow(random.y, (Float)1 / (exponent + 1));
		const Float sin_theta = std::sqrt(1.f - cos_theta * cos_theta);

		return FVector3(
			std::cos(phi) * sin_theta,
			std::sin(phi) * sin_theta,
			cos_theta);
	}

	Float cosine_hemisphere_pdf_phong(const FVector3& aNormal, const FVector3& aDirection) const
	{
		const Float cosTheta = std::max((Float)0, Dot(aNormal, aDirection));
		return (exponent + 1) * std::pow(cosTheta, exponent) * kInv2Pi;
	}

private:
	FColor Ks;
	Float exponent;
};



} // namespace pbrt
