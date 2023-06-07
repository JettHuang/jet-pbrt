// \brief
//		camera
//

#pragma once

#include "pbrt.h"
#include "geometry.h"
#include "sampler.h"


namespace pbrt
{

/*
  camera space:

  y (0, 1, 0)         z(0, 0, 1)
        |            /
        |          /
        |        /
        |      /
        |    /
        |  /
        |/_ _ _ _ _ _ x(1, 0, 0)
        o

  features:
    generate ray
*/
class FCamera
{
public:
    virtual ~FCamera() {}

    FCamera(const FVector3 &ipos, const FVector3& ifront, const FVector3& iup, Float ifov, const FVector2& iresolution)
        : pos(ipos)
		, front(ifront.Normalize())
		, up(iup.Normalize())
        , resolution(iresolution)
    {
		// https://github.com/infancy/pbrt-v3/blob/master/src/core/transform.cpp#L394-L397

		Float tan_fov = std::tan(Degree2Rad(ifov) / 2);

		// left hand, clockwise
		right = up.Cross(front).Normalize() * (tan_fov * Aspect());
        up = front.Cross(right).Normalize()* tan_fov;
    }

    // generate primary ray from camera
    virtual FRay GenerateRay(const FCameraSample& sample) const
    {
        FVector3 dir = front + right * (sample.posfilm.x / resolution.x - (Float)0.5)
            + up * ((Float)0.5 - sample.posfilm.y / resolution.y);
    
        return FRay(pos, dir.Normalize());
    }

protected:
    Float Aspect() const { return resolution.x / resolution.y; }

protected:
    FVector3 pos;
    FVector3 front;
    FVector3 right;
    FVector3 up;

    FVector2 resolution;
};




} // namespace pbrt
