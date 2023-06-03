// \brief
//		primitives
//

#pragma once

#include "pbrt.h"
#include "geometry.h"
#include "shape.h"
#include "material.h"
#include "light.h"


namespace pbrt
{

	
	// primitive in scene
	class FPrimitive
	{
	public:
		const FShape* shape;
		const FMaterial* material;
		const FAreaLight* arealight;

		FPrimitive()
			: shape(nullptr)
			, material(nullptr)
			, arealight(nullptr)
		{}

		FPrimitive(const FShape* inShape, const FMaterial* inMaterial, const FAreaLight* inLight)
			: shape(inShape)
			, material(inMaterial)
			, arealight(inLight)
		{}

		virtual bool Intersect(const FRay& ray, FIntersection& oisect) const
		{
			bool bHit = shape->Intersect(ray, oisect);
			if (bHit)
			{
				oisect.primitive = this;
			}

			return bHit;
		}

		virtual FBounds3 WorldBounds() const
		{
			return shape->WorldBounds();
		}

		std::unique_ptr<FBSDF> GetBsdf(const FIntersection& isect) const
		{
			return material ? material->Scattering(isect) : nullptr;
		}

		FColor GetLe(const FIntersection& isect) const
		{
			return arealight ? arealight->Le(FLightIntersection(isect.position, isect.normal), isect.wo) : FColor::Black;
		}
	};

	// triangle mesh
	class FTriangleMesh : public FPrimitive
	{
	public:
		FTriangleMesh() = default;

		virtual bool Intersect(const FRay& ray, FIntersection& oisect) const override
		{
			bool bHasHit = false;

			for (size_t i=0; i<triangles.size(); ++i)
			{
				bool bHit = triangles[i].Intersect(ray, oisect);
				bHasHit |= bHit;
			}

			if (bHasHit)
			{
				oisect.primitive = this;
			}

			return bHasHit;
		}

		virtual FBounds3 WorldBounds() const override
		{
			return worldbbox;
		}

		static std::shared_ptr<FTriangleMesh> LoadTriangleMesh(const char* filename, const FMaterial* inMaterial);

	protected:

		FBounds3 worldbbox;
		std::vector<FTriangle>	triangles;
	};

} // namespace pbrt