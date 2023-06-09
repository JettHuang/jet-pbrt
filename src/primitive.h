// \brief
//		primitives
//

#pragma once

#include "pbrt.h"
#include "geometry.h"
#include "shape.h"
#include "material.h"
#include "light.h"
#include "bvh.h"


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

		virtual const FBounds3& WorldBounds() const
		{
			return shape->WorldBounds();
		}

		std::unique_ptr<FBSDF> GetBsdf(const FIntersection& isect) const
		{
			return material ? material->Scattering(isect) : nullptr;
		}

		FColor GetLe(const FIntersection& isect) const
		{
			return arealight ? arealight->L(FLightIntersection(isect.position, isect.normal), isect.wo) : FColor::Black;
		}
	};

} // namespace pbrt
