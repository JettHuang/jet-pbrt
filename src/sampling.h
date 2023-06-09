// \brief
//		sampling algorithm
//

#pragma once

#include "pbrt.h"
#include "geometry.h"


namespace pbrt
{

// https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
// https://github.com/mmp/pbrt-v3/blob/master/src/core/sampling.cpp

inline FPoint2 uniform_disk_sample(const FFloat2& uv)
{
	Float radius = std::sqrt(uv.x);
	Float theta = 2 * kPi * uv.y;

	return radius * FPoint2(std::cos(theta), std::sin(theta));
}

inline FPoint2 concentric_disk_sample(FFloat2 uv)
{
	// map uniform random number to [-1, +1]
	uv = 2.f * uv - FFloat2(1, 1);

	// handle degeneracy
	if (uv.x == 0 && uv.y == 0)
	{
		return FPoint2(0, 0);
	}

	// apply concentric mapping to point
	Float radius, theta;
	if (std::abs(uv.x) > std::abs(uv.y))
	{
		radius = uv.x;
		theta = kPiOver4 * (uv.y / uv.x);
	}
	else
	{
		radius = uv.y;
		theta = kPiOver2 - kPiOver4 * (uv.x / uv.y);
	}

	return radius * FPoint2(std::cos(theta), std::sin(theta));
}

// cosine-weighted hemisphere sampling
inline FVector3 cosine_hemisphere_sample(const FFloat2 &uv)
{
	FPoint2 p = concentric_disk_sample(uv);
	Float z = std::sqrt(std::max((Float)0, 1 - p.x * p.x - p.y * p.y));

	return FVector3(p.x, p.y, z);
}

inline Float cosine_hemisphere_pdf(Float costheta)
{
	return costheta * kInvPi;
}

inline FVector3 uniform_hemisphere_sample(const FFloat2& uv)
{
	Float z = uv.x; // [0, 1)
	Float radius = std::sqrt(std::max((Float)0, (Float)1 - z * z));
	Float phi = 2 * kPi * uv.y;

	return FVector3(radius * std::cos(phi), radius * std::sin(phi), z);
}

inline Float uniform_hemisphere_pdf()
{
	return kInv2Pi;
}

inline FVector3 uniform_sphere_sample(const FFloat2& uv)
{
	Float z = 1 - 2 * uv.x; // (-1, 1)
	Float radius = std::sqrt(std::max((Float)0, (Float)1 - z * z));
	Float phi = 2 * kPi * uv.y;

	return FVector3(radius * std::cos(phi), radius * std::sin(phi), z);
}

inline Float uniform_sphere_pdf() { return kInv4Pi; }


/*

		/         _
	   /        / O \
	  /         O O O (a sphere)
	 /       .  \ O /
	/    .
   / .     theta
  . _ _ _ _ _ _ _ _

*/
inline FVector3 uniform_cone_sample(const FFloat2& uv, Float cos_theta_max)
{
	Float cos_theta = (1 - uv.x) + uv.x * cos_theta_max;
	Float sin_theta = std::sqrt((Float)1 - cos_theta * cos_theta);

	Float phi = uv.y * 2 * kPi;

	return FVector3(
		std::cos(phi) * sin_theta,
		std::sin(phi) * sin_theta,
		cos_theta);
}

inline Float uniform_cone_pdf(Float cos_theta_max)
{
	return 1 / (2 * kPi * (1 - cos_theta_max));
}

inline FPoint2 uniform_triangle_sample(const FFloat2& uv)
{
	Float su0 = std::sqrt(uv.x);
	return FPoint2(1 - su0, uv.y * su0);
}


inline Float balance_heuristic(int f_num, Float f_pdf, int g_num, Float g_pdf)
{
	return (f_num * f_pdf) / (f_num * f_pdf + g_num * g_pdf);
}

inline Float power_heuristic(int f_num, Float f_pdf, int g_num, Float g_pdf)
{
	Float f = f_num * f_pdf, g = g_num * g_pdf;
	return (f * f) / (f * f + g * g);
}


} // namespace pbrt

