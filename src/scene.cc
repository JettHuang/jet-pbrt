// \brief
//		scene.cc
//

#include "scene.h"


namespace pbrt
{

void FScene::Preprocess()
{
	CalculateWorldBound();

	for (std::shared_ptr<FLight>& light : lights)
	{
		light->Preprocess(*this);
	} // end for 

	// build bvh
	bvh = std::make_shared<FBVH_Node<FPrimitive*>>(shadow_primitives, 0, shadow_primitives.size());
	shadow_bvh = bvh.get();
}

bool FScene::Intersect(const FRay& ray, FIntersection& oisect) const
{
	if (shadow_bvh)
	{
		return shadow_bvh->Intersect(ray, oisect);
	}

	return false;
}

void FScene::CalculateWorldBound()
{
	FBounds3 bound;

    for (auto primitive : primitives)
	{
        bound.Expand(primitive->WorldBounds());
	}

	worldBound = bound;
}

//////////////////////////////////////////////////////////////////////////

std::vector<std::shared_ptr<FShape>> FScene::CreateTriangleMesh(const char* filename, bool flip_normal, bool bFlipHandedness, const FVector3& offset, Float inScale)
{
	std::vector<std::shared_ptr<FTriangle>> mesh;
	std::vector<std::shared_ptr<FShape>> newshapes;

	if (LoadTriangleMesh(filename, mesh, flip_normal, bFlipHandedness, offset, inScale))
	{
		for (auto triangle : mesh)
		{
			shapes.push_back(triangle);
			newshapes.push_back(triangle);
		} // end for 
	}

	return newshapes;
}

std::vector<std::shared_ptr<FPrimitive>> FScene::CreatePrimitives(const std::vector<std::shared_ptr<FShape>>& inMesh, const std::shared_ptr<FMaterial>& inMaterial)
{
	std::vector<std::shared_ptr<FPrimitive>> newprimitives;

	for (auto triangle : inMesh)
	{
		std::shared_ptr<FPrimitive> newprimitive = CreatePrimitive(triangle.get(), inMaterial.get(), nullptr);
		newprimitives.push_back(newprimitive);
	}

	return newprimitives;
}

std::vector<std::shared_ptr<FAreaLight>> FScene::CreateAreaLights(int samplesNum, const FColor& radiance, const std::vector<std::shared_ptr<FShape>>& inShapes, const std::shared_ptr<FMaterial>& inMaterial)
{
	std::vector<std::shared_ptr<FAreaLight>> newlights;

	for (auto shape : inShapes)
	{
		std::shared_ptr<FAreaLight> newlight = CreateAreaLight(samplesNum, radiance, shape, inMaterial);
	} // end for

	return newlights;
}

std::shared_ptr<FAreaLight> FScene::CreateAreaLight(int samplesNum, const FColor& radiance, const std::shared_ptr<FShape>& inShape, const std::shared_ptr<FMaterial>& inMaterial)
{
	std::shared_ptr<FAreaLight> areaLight = CreateLight<FAreaLight>(FPoint3(0,0,0), samplesNum, radiance, inShape.get());

	CreatePrimitive(inShape.get(), inMaterial.get(), areaLight.get());
	return areaLight;
}


} // namespace pbrt
