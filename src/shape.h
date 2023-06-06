// \brief
//		shape
//

#pragma once

#include "pbrt.h"
#include "geometry.h"
#include "sampling.h"
#include "bsdf.h"

namespace pbrt
{

class FPrimitive;
class FAreaLight;
class FMaterial;
class FBSDF;

/*
  prev   n   light
  ----   ^   -----
	^    |    ^
	 \   | ¦È /
   wo \  |  / wi is unknown, sampling from bsdf or light
	   \ | /
		\|/
	  -------
	   isect
*/
// surface intersection
class FIntersection
{
public:
	FPoint3		position; // world position of intersection
	FNormal3	normal;
	FVector3	wo;

	const FPrimitive* primitive;

	// constructor
	FIntersection()
		: primitive(nullptr)
	{}

	FIntersection(const FPoint3 &pos, const FNormal3 &n, const FVector3& wo)
		: position(pos)
		, normal(n)
		, wo(wo)
		, primitive(nullptr)
	{}

	const FPrimitive* Primitive() const { return primitive; }

	std::unique_ptr<FBSDF> Bsdf() const;
	FColor Le() const;

	// spawn a new ray start from this intersection to the direction
	FRay SpawnRay(const FVector3& dir) const
	{
		return FRay(position, dir);
	}

	// spawn ray to target
	FRay SpawnRayTo(const FPoint3& target) const
	{
		return SpawnRay(Normalize(target - position));
	}

	// spawn ray to other isect
	FRay SpawnRayTo(const FIntersection& isect) const
	{
		return SpawnRay(Normalize(isect.position - position));
	}
};

// sample point on light surface
struct FLightIntersection
{
	FPoint3		position; // world position of intersection on light surface
	FNormal3	normal;

	FLightIntersection() = default;
	FLightIntersection(const FPoint3 &p, const FNormal3& n)
		: position(p)
		, normal(n)
	{}
};


/*
     z(0, 0, 1)
          |
          | theta/
          |    /
          |  /
          |/_ _ _ _ _ _ x(1, 0, 0)
         / \
        / phi\
       /       \
      /          \
 y(0, 1, 0)

   https://www.pbr-book.org/3ed-2018/Shapes/Spheres
*/
class FShape
{
public:
    virtual ~FShape() = default;

    virtual bool Intersect(const FRay &ray, FIntersection &oisect) const = 0;

	const FBounds3& WorldBounds() const { return worldBox; }
    virtual Float Area() const = 0;

public:
    // these methods below only used for `area_light_t`
    virtual FLightIntersection SamplePosition(const FFloat2& random, Float * out_pdf) const = 0;


    // default compute `*_direction` by `*_position` 
    virtual FLightIntersection SampleDirection(const FIntersection & isect, const FFloat2& random, Float * out_pdf_direction) const
    {
        FLightIntersection light_isect = SamplePosition(random, out_pdf_direction);
        FVector3 wi = light_isect.position - isect.position;
        Float dist2 = wi.Length2();

        if (dist2 == 0)
        {
            *out_pdf_direction = 0;
        }
        else
        {
            wi = Normalize(wi);
            // look comments in `pdf_direction()` below
            *out_pdf_direction *= dist2 / AbsDot(light_isect.normal, -wi);

            if (std::isinf(*out_pdf_direction))
                *out_pdf_direction = 0;
        }

        return light_isect;
    }

    virtual Float Pdf_Direction(const FIntersection & isect, const FVector3& world_wi) const
    {
        FRay ray = isect.SpawnRay(world_wi);
        FIntersection isect_onlight;

        if (!Intersect(ray, isect_onlight))
            return 0;

        /*
          convert light sample point to solid angle:

              $$\mathrm{d} \omega = \fact{\mathrm{d} A^{\perp} }{l^{2}} $$

          because:
              unit_solid_angle = 1 / distance_squared(...)
              projected_light_area = abs_dot(...) * area()
              projected_solid_angle = projected_light_area / distance_squared

              pdf = distance_squared(...) / (abs_dot(...) * area()) = inverse_projected_solid_angle
              1 / pdf
                      = (abs_dot(...) * area()) / distance_squared(...) = projected_solid_angle
                      = (1 / distance_squared(...)) * (abs_dot(...) * area()) = unit_solid_angle * projected_light_area

          so:
              (f * Li * cos_theta) / pdf = f * (Li * cos_theta * projected_solid_angle)

          or:
              (f * Li * cos_theta) / pdf = f * (Li * cos_theta * unit_solid_angle * projected_light_area)
        */
        Float pdf = Distance2(isect.position, isect_onlight.position) / (AbsDot(isect_onlight.normal, -world_wi) * Area());
        if (std::isinf(pdf))
            pdf = 0.f;

        return pdf;
    }

public:
	FBounds3 worldBox;
};


// disk
class FDisk : public FShape
{
public:
    FDisk(const FPoint3 &pos, const FNormal3& normal, Float radius) 
        : position(pos)
		, normal(Normalize(normal))
		, radius(radius)
	{
		worldBox = CalcWorldBounds();
	}

    bool Intersect(const FRay& ray, FIntersection& oisect) const override
	{
		if (isEqual(Dot(ray.Dir(), normal), (Float)0))
			return false;

		const FVector3 op = position - ray.Origin();
		const Float distance = Dot(normal, op) / Dot(normal, ray.Dir());

		if ((distance > ray.MinT()) && (distance < ray.MaxT()))
		{
			FPoint3 hit_point = ray(distance);
			if (Distance(position, hit_point) <= radius)
			{
				ray.SetMaxT(distance);
				oisect = FIntersection(hit_point, normal, -ray.Dir());

				return true;
			}
		}

		return false;
	}

	FBounds3 CalcWorldBounds() const
	{
		FFrame frame(normal);
        FVector3 rb = frame.Binormal() * radius;
        FVector3 rt = frame.Tangent() * radius;

        FBounds3 bbox(position + rb + rt);
        bbox = bbox.Join(position + rb - rt);
        bbox = bbox.Join(position - rb - rt);
        bbox = bbox.Join(position - rb + rt);

		return bbox;
	}

	Float Area() const override { return kPi * radius * radius; }

public:
    FLightIntersection SamplePosition(const FFloat2& random, Float* out_pdf) const override
	{
        FLightIntersection light_isect;

		FFrame frame(normal);
		FPoint2 sample_point = concentric_disk_sample(random);
		light_isect.position = position + radius * (frame.Binormal() * sample_point.x + frame.Tangent() * sample_point.y);

		light_isect.normal = normal;

		*out_pdf = 1 / Area();
		return light_isect;
	}

public:
	FPoint3  position;
	FNormal3 normal;
	Float    radius;
};

// triangle shape
class FTriangle : public FShape
{
public:
	FTriangle(const FPoint3 &p0, const FPoint3 &p1, FPoint3 &p2, bool flip_normal = false)
		: p0(p0), p1(p1), p2(p2)
	{
		normal = Normalize(Cross(p1 - p0, p2 - p0));
		if (flip_normal)
			normal = -normal;

		worldBox = CalcWorldBounds();
	}

	bool Intersect(const FRay& ray, FIntersection& oisect) const override
	{
		// https://github.com/SmallVCM/SmallVCM/blob/master/src/geometry.hxx#L125-L156

		const FVector3 oa = p0 - ray.Origin();
		const FVector3 ob = p1 - ray.Origin();
		const FVector3 oc = p2 - ray.Origin();

		const FVector3 v0 = Cross(oc, ob);
		const FVector3 v1 = Cross(ob, oa);
		const FVector3 v2 = Cross(oa, oc);

		const Float v0d = Dot(v0, ray.Dir());
		const Float v1d = Dot(v1, ray.Dir());
		const Float v2d = Dot(v2, ray.Dir());

		if (((v0d < 0) && (v1d < 0) && (v2d < 0)) ||
			((v0d >= 0) && (v1d >= 0) && (v2d >= 0)))
		{
			// 1. first calculate the vertical distance from ray.origin to the plane,
			//    by `dot(normal, op)` (or `bo`, `co`)
			// 2. then calculate the distance from ray.origin to the plane alone ray.direction, 
			//    by `distance * dot(normal, ray.direction()) = vertical_distance`
			const Float distance = Dot(normal, oa) / Dot(normal, ray.Dir());

			if ((distance > ray.MinT()) && (distance < ray.MaxT()))
			{
				ray.SetMaxT(distance);
				FPoint3 hit_point = ray(distance);
				oisect = FIntersection(hit_point, normal, -ray.Dir());

				return true;
			}
		}

		return false;
	}

	FBounds3 CalcWorldBounds() const
	{
		FBounds3 bbox(p0, p1);
		bbox = bbox.Join(p2);

		return bbox;
	}

	Float Area() const override { return (Float)0.5 * Cross(p1 - p0, p2 - p0).Length(); }

	FLightIntersection SamplePosition(const FFloat2& random, Float* out_pdf) const override
	{
		FPoint2 b = uniform_triangle_sample(random);

		FLightIntersection light_isect;
		light_isect.position = b.x * p0 + b.y * p1 + (1 - b.x - b.y) * p2;
		light_isect.normal = normal;

		*out_pdf = 1 / Area();
		return light_isect;
	}

public:
	FPoint3 p0, p1, p2;
	FNormal3 normal;
};

// rectangle
//    p0------------p3
//     |            |
//     |            |
//    p1------------p2
class FRectangle : public FShape
{
public:
	FRectangle(const FPoint3& p0, const FPoint3& p1, const FPoint3& p2, const FPoint3& p3, bool flip_normal = false)
		: p0(p0), p1(p1), p2(p2), p3(p3)
	{
		// TODO: check points on the same plane

		normal = Normalize(Cross(p1 - p0, p2 - p0));
		if (flip_normal)
			normal = -normal;

		worldBox = CalcWorldBounds();
	}

    static FRectangle FromXY(Float x0, Float x1, Float y0, Float y1, Float z, bool flip_normal = false);
    static FRectangle FromXZ(Float x0, Float x1, Float z0, Float z1, Float y, bool flip_normal = false);
    static FRectangle FromYZ(Float y0, Float y1, Float z0, Float z1, Float x, bool flip_normal = false);

	bool Intersect(const FRay& ray, FIntersection& oisect) const override
	{
		// https://github.com/SmallVCM/SmallVCM/blob/master/src/geometry.hxx#L125-L156

		const FVector3 oa = p0 - ray.Origin();
		const FVector3 ob = p1 - ray.Origin();
		const FVector3 oc = p2 - ray.Origin();
		const FVector3 od = p3 - ray.Origin();

		const FVector3 v0 = Cross(oc, ob);
		const FVector3 v1 = Cross(ob, oa);
		const FVector3 v2 = Cross(oa, od);
		const FVector3 v3 = Cross(od, oc);

		const Float v0d = Dot(v0, ray.Dir());
		const Float v1d = Dot(v1, ray.Dir());
		const Float v2d = Dot(v2, ray.Dir());
		const Float v3d = Dot(v3, ray.Dir());

		if (((v0d < 0) && (v1d < 0) && (v2d < 0) && (v3d < 0)) ||
			((v0d >= 0) && (v1d >= 0) && (v2d >= 0) && (v3d >= 0)))
		{
			const Float distance = Dot(normal, oa) / Dot(normal, ray.Dir());

			if ((distance > ray.MinT()) && (distance < ray.MaxT()))
			{
				ray.SetMaxT(distance);
				FPoint3 hit_point = ray(distance);
				FNormal3 N = Dot(normal, ray.Dir()) <= 0 ? normal : -normal;
				oisect = FIntersection(hit_point, N, -ray.Dir());

				return true;
			}
		}

		return false;
	}

	FBounds3 CalcWorldBounds() const
	{
		return FBounds3(p0, p1).Join(p2).Join(p3);
	}

	Float Area() const override { return Cross(p0 - p1, p2 - p1).Length(); }

	FLightIntersection SamplePosition(const FFloat2& random, Float* out_pdf) const override
	{
		FLightIntersection light_isect;
		light_isect.position = p1 + (p0 - p1) * random.x + (p2 - p1) * random.y;
		light_isect.normal = normal;

		*out_pdf = 1 / Area();
		return light_isect;
	}

public:
	FPoint3 p0, p1, p2, p3;
	FNormal3 normal;
};


// sphere
class FSphere : public FShape
{
public:
    FSphere(const FVector3& center, Float r) 
        : center(center)
        , radius(r)
        , radius2(r * r)
    {
		worldBox = CalcWorldBounds();
    }

    bool Intersect(const FRay& ray, FIntersection& oisect) const override
    {
		FVector3 oc = ray.Origin() - center;
		auto a = ray.Dir().Length2();
		auto half_b = Dot(oc, ray.Dir());
		auto c = oc.Length2() - radius * radius;
		auto discriminant = half_b * half_b - a * c;

		auto t_max = ray.MaxT();
		auto t_min = ray.MinT();

		if (discriminant > 0.0) {
			Float root = sqrt(discriminant);
			Float time = 0.0f;

			// check root1
			auto root1 = (-half_b - root) / a;
			if (root1 < t_max && root1 > t_min) {
				time = root1;
			}
			else {
				// check root2
				auto root2 = (-half_b + root) / a;
				if (root2 < t_max && root2 > t_min) {
					time = root2;
				}
				else {
					return false;
				}
			}

			ray.SetMaxT(time);
			FPoint3 hit_point = ray(time);
			oisect = FIntersection(hit_point, (hit_point - center).Normalize(), -ray.Dir());
			return true;
		}

		return false;
    }

    FBounds3 CalcWorldBounds() const
    {
        FVector3 half(radius, radius, radius);
        return FBounds3(center + half, center - half);
    }

    Float Area() const override { return 4 * kPi * radius2; }

public:
    FLightIntersection SamplePosition(const FFloat2& random, Float* out_pdf) const override
    {
        FVector3 direction = uniform_sphere_sample(random);
        FPoint3 position = center + radius * direction;

        FLightIntersection light_isect;
        light_isect.position = position;
        light_isect.normal = Normalize(direction);

        *out_pdf = 1 / Area();

        return light_isect;
    }

    // TODO: confirm
    FLightIntersection SampleDirection(const FIntersection& isect, const FFloat2& random, Float* out_pdf_direction) const override
    {
        // station 1: In or On the sphere
        if (Distance2(isect.position, center) <= radius * radius)
        {
            FLightIntersection light_isect = SamplePosition(random, out_pdf_direction);
            FVector3 wi = light_isect.position - isect.position;
            Float dist2 = wi.Length2();

            if (wi.Length2() == 0)
                *out_pdf_direction = 0;
            else
            {
                // convert from area measure returned by Sample() call above to solid angle measure.
                wi = Normalize(wi);
                *out_pdf_direction *= Distance2(light_isect.position, isect.position) / AbsDot(isect.normal, -wi);
            }

            if (std::isinf(*out_pdf_direction))
                *out_pdf_direction = 0;

            return light_isect;
        }

        // sample sphere uniformly inside subtended cone

        /*
                /         _
               /        / O \
              /         O O O (a sphere)
             /       .  \ O /
            /    .
           / .     theta
          . _ _ _ _ _ _ _ _

        */

        Float dist = Distance(isect.position, center);
        Float inv_dist = 1 / dist;

        // compute $\theta$ and $\phi$ values for sample in cone
        Float sin_theta_max = radius * inv_dist;
        Float sin_theta_max_sq = sin_theta_max * sin_theta_max;
        Float inv_sin_theta_max = 1 / sin_theta_max;
        Float cos_theta_max = std::sqrt(std::max((Float)0.f, 1 - sin_theta_max_sq));

        Float cos_theta = (cos_theta_max - 1) * random.x + 1;
        Float sin_theta_sq = 1 - cos_theta * cos_theta;

        if (sin_theta_max_sq < 0.00068523f /* sin^2(1.5 deg) */)
        {
            /* fall back to a Taylor series expansion for small angles, where
               the standard approach suffers from severe cancellation errors */
            sin_theta_sq = sin_theta_max_sq * random.x;
            cos_theta = std::sqrt(1 - sin_theta_sq);
        }

        // compute angle $\alpha$ from center of sphere to sampled point on surface
        Float cos_alpha = sin_theta_sq * inv_sin_theta_max +
            cos_theta * std::sqrt(std::max((Float)0.f, 1.f - sin_theta_sq * inv_sin_theta_max * inv_sin_theta_max));
        Float sin_alpha = std::sqrt(std::max((Float)0.f, 1.f - cos_alpha * cos_alpha));
        Float phi = random.y * 2 * kPi;

        // compute coordinate system for sphere sampling
        FVector3 normal = (center - isect.position) * inv_dist;
        FFrame frame(normal);

        // compute surface normal and sampled point on sphere
        FVector3 world_normal =
            Spherical_2_Direction(sin_alpha, cos_alpha, phi, -frame.Binormal(), -frame.Tangent(), -frame.Normal());
        FPoint3 world_position = center + radius * FPoint3(world_normal.x, world_normal.y, world_normal.z);

        FLightIntersection light_isect;
        light_isect.position = world_position;
        light_isect.normal = world_normal;

        // uniform cone PDF.
        *out_pdf_direction = 1 / (2 * kPi * (1 - cos_theta_max));

        return light_isect;
    }

    Float Pdf_Direction(const FIntersection& isect, const FVector3& world_wi) const override
    {
        // return uniform PDF if point is inside sphere
        if (Distance2(isect.position, center) <= radius * radius)
            return FShape::Pdf_Direction(isect, world_wi);

        // compute general sphere PDF
        Float sin_theta_max_sq = radius * radius / Distance2(isect.position, center);
        Float cos_theta_max = std::sqrt(std::max((Float)0, 1 - sin_theta_max_sq));
        return uniform_cone_pdf(cos_theta_max);
    }

private:
    FVector3 center;
    Float radius;
    Float radius2;
};



} // namespace pbrt

