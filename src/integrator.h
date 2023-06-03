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

    void Render(const FScene* scene, FSampler* sampler, FFilm* film);

    virtual FColor Li(const FRay& ray, const FScene* scene, FSampler* sampler) = 0;
};


// debuger shapes
class FDebugIntegrator : public FIntegrator
{
public:
	FColor Li(const FRay& ray, const FScene* scene, FSampler* sampler) override
	{
		FIntersection isect;
		if (scene->Intersect(ray, isect))
		{
            return isect.position.Normalize();
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

    FColor Li(const FRay& ray, const FScene* scene, FSampler* sampler) override
    {
        return Li(ray, scene, sampler, 0);
    }

protected:
    FColor Li(const FRay& ray, const FScene* scene, FSampler* sampler, int depth);

    FColor SpecularReflect(const FRay& ray, const FIntersection& isect, const FBSDF* bsdfptr, const FScene* scene, FSampler* sampler, int depth);
    FColor SpecularTransmit(const FRay& ray, const FIntersection& isect, const FBSDF* bsdfptr, const FScene* scene, FSampler* sampler, int depth);

protected:
	int maxDepth;
};


} // namespace pbrt
