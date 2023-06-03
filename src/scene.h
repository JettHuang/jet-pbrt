// \brief
//		scene
//

#pragma once

#include "pbrt.h"
#include "geometry.h"
#include "shape.h"
#include "light.h"
#include "material.h"
#include "primitive.h"
#include "camera.h"


namespace pbrt
{


// class scene
class FScene
{
public:
	FScene() {}

	void Preprocess();

	bool Intersect(const FRay& ray, FIntersection& isect) const;
	bool Occluded(const FPoint3& pos, const FNormal3& normal, const FVector3& dir, Float dist) const
	{
		FRay ray(pos, dir);
		FIntersection unused;

		return Intersect(ray, unused);
	}

	bool Occluded(const FIntersection& isect1, const FPoint3& target) const
	{
		return Occluded(isect1.position, isect1.normal, Normalize(target - isect1.position), Distance(isect1.position, target));
	}

	bool Occluded(const FIntersection& isect1, const FIntersection& isect2) const
	{
		return Occluded(isect1.position, isect1.normal, Normalize(isect2.position - isect1.position), Distance(isect1.position, isect2.position));
	}

	FBounds3 WorldBound() const
	{
		return worldBound;
	}

	const FCamera* Camera() const { return camera.get(); }
	int LightNum() const { return (int)lights.size(); }
	const std::vector<std::shared_ptr<FLight>>& Lights() const { return lights; }

	const FEnvironmentLight* EnvironmentLight() const { return environment_light.get(); }
	FColor EnvironmentLighting(const FRay& ray) const
	{
		if (environment_light != nullptr)
		{
			return environment_light->Le(ray);
		}

		return FColor::Black;
	}

protected:
	void CalculteWorldBound();

public:
	std::shared_ptr<FCamera>	camera;
	std::vector<std::shared_ptr<FShape>>	shapes;
	std::vector<std::shared_ptr<FMaterial>> materials;

	std::vector<std::shared_ptr<FLight>> lights;
	std::shared_ptr<FEnvironmentLight> environment_light;

	std::vector<std::shared_ptr<FPrimitive>> primitives;

	FBounds3 worldBound;
};


std::shared_ptr<FScene> create_cornellbox_scene(const FVector2& filmsize);
std::shared_ptr<FScene> create_random_scene(const FVector2& filmsize);

} // namespace pbrt
