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
#include "bvh.h"


namespace pbrt
{


// class scene
class FScene
{
public:
	FScene(const char *inName)
		: name(inName)
		, shadow_camera(nullptr)
		, shadow_bvh(nullptr)
	{}

	const char* NameStr() const { return name.c_str(); }

	void Preprocess();

	bool Intersect(const FRay& ray, FIntersection& oisect) const;
	bool Occluded(const FPoint3& pos, const FNormal3& normal, const FVector3& dir, Float dist) const
	{
		FRay ray(pos, dir, 0.001f, dist - 0.001f);
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

	const FCamera* Camera() const { return shadow_camera; }
	int LightNum() const { return (int)shadow_lights.size(); }
	const std::vector<FLight*>& Lights() const { return shadow_lights; }
	const std::vector<FLight*>& InfiniteLights() const { return shadow_infinitelights; }

	//////////////////////////////////////////////////////////////////////////
	// add interfaces
	template<typename T, typename ...U>
	std::shared_ptr<T> CreateCamera(const U& ...args)
	{
		std::shared_ptr<T> cam = std::make_shared<T>(args...);
		camera = cam;
		shadow_camera = cam.get();
		return cam;
	}

	template<typename T, typename ...U>
	std::shared_ptr<T> CreateShape(const U& ... args)
	{
		std::shared_ptr<T> shape = std::make_shared<T>(args...);

		shapes.push_back(shape);
		return shape;
	}

	template<typename T, typename ...U>
	std::shared_ptr<T> CreateMaterial(const U& ... args)
	{
		std::shared_ptr<T> mat = std::make_shared<T>(args...);

		materials.push_back(mat);
		return mat;
	}

	template<typename T, typename ...U>
	std::shared_ptr<T> CreateLight(const U& ... args)
	{
		std::shared_ptr<T> light = std::make_shared<T>(args...);

		lights.push_back(light);
		shadow_lights.push_back(light.get());

		if (light->Flags() & eLightFlags::InfiniteLight)
		{
			shadow_infinitelights.push_back(light.get());
		}

		return light;
	}

	template<typename ...U>
	std::shared_ptr<FPrimitive> CreatePrimitive(const U& ... args)
	{
		std::shared_ptr<FPrimitive> primitive = std::make_shared<FPrimitive>(args...);

		primitives.push_back(primitive);
		shadow_primitives.push_back(primitive.get());

		return primitive;
	}

	std::vector<std::shared_ptr<FShape>> CreateTriangleMesh(const char* filename, bool flip_normal = false, bool bFlipHandedness = false, const FVector3 & offset = FVector3(0, 0, 0), Float inScale = 1.f);
	std::vector<std::shared_ptr<FPrimitive>> CreatePrimitives(const std::vector<std::shared_ptr<FShape>> &inMesh, const std::shared_ptr<FMaterial>& inMaterial);

	std::vector<std::shared_ptr<FAreaLight>> CreateAreaLights(int samplesNum, const FColor& radiance, const std::vector<std::shared_ptr<FShape>> & inShapes, const std::shared_ptr<FMaterial>& inMaterial);
	std::shared_ptr<FAreaLight> CreateAreaLight(int samplesNum, const FColor& radiance, const std::shared_ptr<FShape> &inShape, const std::shared_ptr<FMaterial>& inMaterial);

protected:
	void CalculateWorldBound();

public:
	std::string  name;
	std::shared_ptr<FCamera>	camera;
	std::vector<std::shared_ptr<FShape>>	shapes;
	std::vector<std::shared_ptr<FMaterial>> materials;

	std::vector<std::shared_ptr<FLight>> lights;

	std::vector<std::shared_ptr<FPrimitive>> primitives;

	// shadows for multi-thread visiting
	FCamera* shadow_camera;
	std::vector<FLight*> shadow_lights;
	std::vector<FLight*> shadow_infinitelights;
	std::vector<FPrimitive*> shadow_primitives;

	FBounds3 worldBound;
	
	// bvh 
	std::shared_ptr<FBVH_NodeBase>  bvh;
	FBVH_NodeBase* shadow_bvh;
};


} // namespace pbrt
