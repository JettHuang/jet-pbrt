// \brief
//		integrator
//

/*
  Li = Lo = Le + ¡ÒLi
          = Le + ¡Ò(Le + ¡ÒLi)
          = Le + ¡ÒLe  + ¡Ò¡ÒLi
          = Le + ¡ÒLe  + ¡Ò¡Ò(Le + ¡ÒLi)
          = Le + ¡ÒLe  + ¡Ò¡ÒLe  + ¡Ò¡Ò¡Ò(Le + ¡ÒLi)
          = Le + ¡ÒLe  + ¡Ò¡ÒLe  + ¡Ò¡Ò¡ÒLe  + ¡Ò¡Ò¡Ò¡ÒLe + ...
*/

#include "pbrt.h"
#include "scene.h"
#include "film.h"


namespace pbrt
{


 /*
  rendering scene by Rendering Equation(Li = Lo = Le + ¡ÒLi)
  solving Rendering Equation(a integral equation) by numerical integration(ie. Monte Carlo Integration)
*/
class FIntegrator
{
public:
    virtual ~FIntegrator() {}

    void Render(const FScene* scene, FSampler* sampler, FFilm* film, int numthreads = 1) const;

protected:
    void DoRender(const FScene* scene, FSampler* sampler, FFilmView *filmview) const;

    virtual FColor Li(const FRay& ray, const FScene* scene, FSampler* sampler) const = 0;

    friend class FRenderTask;
};


// debuger shapes
class FDebugIntegrator : public FIntegrator
{
public:
	FColor Li(const FRay& ray, const FScene* scene, FSampler* sampler) const override
	{
		FIntersection isect;
		if (scene->Intersect(ray, isect))
		{
            return FColor(std::abs(isect.normal.x), std::abs(isect.normal.y), std::abs(isect.normal.z));
		}

        return FColor::Black;
	}
};


// WhittedIntegrator 
class FWhittedIntegrator : public FIntegrator
{
public:
    FWhittedIntegrator(int maxDepth)
        : maxDepth(maxDepth)
	{
	}

    FColor Li(const FRay& ray, const FScene* scene, FSampler* sampler) const override
    {
        return Li(ray, scene, sampler, 0);
    }

protected:
    FColor Li(const FRay& ray, const FScene* scene, FSampler* sampler, int depth) const;

    FColor SpecularReflect(const FRay& ray, const FIntersection& isect, const FBSDF* bsdfptr, const FScene* scene, FSampler* sampler, int depth) const;
    FColor SpecularTransmit(const FRay& ray, const FIntersection& isect, const FBSDF* bsdfptr, const FScene* scene, FSampler* sampler, int depth) const;

protected:
	int maxDepth;
};


} // namespace pbrt
