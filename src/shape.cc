// \brief
//		shape.cc
//

#include "shape.h"
#include "primitive.h"
#include "OBJ_Loader.h"


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

	// load triangles from *.obj file
	bool LoadTriangleMesh(const char* filename, std::vector<std::shared_ptr<FTriangle>>& outTriangles, bool flip_normal, bool bFlipHandedness, const FVector3& offset, Float inScale)
	{
		outTriangles.clear();

		objl::Loader loader;
		if (!loader.LoadFile(filename))
		{
			PBRT_ERROR("load triangle mesh failed. %s", filename);
			return false;
		}

		PBRT_DOCHECK(loader.LoadedMeshes.size() == 1);
		auto mesh = loader.LoadedMeshes[0];

		outTriangles.reserve(mesh.Vertices.size() / 3);
		for (int i = 0; i < mesh.Vertices.size(); i += 3)
		{
			FVector3 v0 = FVector3(mesh.Vertices[i + 0].Position.X, mesh.Vertices[i + 0].Position.Y, mesh.Vertices[i + 0].Position.Z);
			FVector3 v1 = FVector3(mesh.Vertices[i + 1].Position.X, mesh.Vertices[i + 1].Position.Y, mesh.Vertices[i + 1].Position.Z);
			FVector3 v2 = FVector3(mesh.Vertices[i + 2].Position.X, mesh.Vertices[i + 2].Position.Y, mesh.Vertices[i + 2].Position.Z);

			if (bFlipHandedness)
			{
				v0.z = -v0.z;
				v1.z = -v1.z;
				v2.z = -v2.z;
			}

			v0 *= inScale;
			v1 *= inScale;
			v2 *= inScale;

			v0 += offset;
			v1 += offset;
			v2 += offset;
			std::shared_ptr<FTriangle> triangle = std::make_shared<FTriangle>(v0, v1, v2, flip_normal);

			outTriangles.push_back(triangle);
		} // end for i

		return true;
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
