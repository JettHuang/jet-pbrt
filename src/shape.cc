// \brief
//		shape.cc
//

#include "shape.h"
#include "primitive.h"


namespace pbrt
{
	std::unique_ptr<FBSDF> FIntersection::Bsdf() const
	{
		return primitive ? primitive->GetBsdf(*this) : nullptr;
	}

	FColor FIntersection::Le() const
	{
		return primitive ? primitive->GetLe(*this) : FColor::Black;
	}


	// rectangle
	//    p0------------p3
	//     |            |
	//     |            |
	//    p1------------p2
	FRectangle FRectangle::FromXY(Float x0, Float x1, Float y0, Float y1, Float z, bool flip_normal)
	{
		FPoint3 p0(x0, y0, z), p1(x1, y0, z), p2(x1, y1, z), p3(x0, y1, z);

		return FRectangle(p0, p1, p2, p3, flip_normal);
	}

	FRectangle FRectangle::FromXZ(Float x0, Float x1, Float z0, Float z1, Float y, bool flip_normal)
	{
		FPoint3 p0(x0, y, z0), p1(x0, y, z1), p2(x1, y, z1), p3(x1, y, z0);

		return FRectangle(p0, p1, p2, p3, flip_normal);
	}

	FRectangle FRectangle::FromYZ(Float y0, Float y1, Float z0, Float z1, Float x, bool flip_normal)
	{
		FPoint3 p0(x, y0, z0), p1(x, y1, z0), p2(x, y1, z1), p3(x, y0, z1);

		return FRectangle(p0, p1, p2, p3, flip_normal);
	}

}
