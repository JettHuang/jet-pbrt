// \brief
//		geometry.cc
//

#include "geometry.h"

namespace pbrt
{

	bool FBounds3::Intersect(const FRay& ray) const
	{
		const FPoint3& origin = ray.Origin();
		const FVector3& dir = ray.Dir();
		Float tmin = ray.MinT();
		Float tmax = ray.MaxT();

		for (int a = 0; a < 3; a++)
		{
			auto t0 = std::min((_min[a] - origin[a]) / dir[a],
				(_max[a] - origin[a]) / dir[a]);
			auto t1 = std::max((_min[a] - origin[a]) / dir[a],
				(_max[a] - origin[a]) / dir[a]);
			tmin = std::max(t0, tmin);
			tmax = std::min(t1, tmax);
			if (tmax <= tmin)
				return false;
		} // end for a

		return true;
	}


} // namespace pbrt
