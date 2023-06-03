// \brief
//		primitive.cc
//

#include "primitive.h"
#include "OBJ_Loader.h"


namespace pbrt
{

std::shared_ptr<FTriangleMesh> FTriangleMesh::LoadTriangleMesh(const char* filename, const FMaterial* inMaterial)
{
	objl::Loader loader;
	if (!loader.LoadFile(filename))
	{
		ERROR("load triangle mesh failed. %s", filename);
		return nullptr;
	}
	
	std::shared_ptr<FTriangleMesh> triangleMesh = std::make_shared<FTriangleMesh>();

	triangleMesh->material = inMaterial;

	DOCHECK(loader.LoadedMeshes.size() == 1);
	auto mesh = loader.LoadedMeshes[0];


	FBounds3 bound;
	for (int i = 0; i < mesh.Vertices.size(); i += 3) 
	{
		FVector3 v0 = FVector3(mesh.Vertices[i + 0].Position.X, mesh.Vertices[i + 0].Position.Y, mesh.Vertices[i + 0].Position.Z);
		FVector3 v1 = FVector3(mesh.Vertices[i + 1].Position.X, mesh.Vertices[i + 1].Position.Y, mesh.Vertices[i + 1].Position.Z);
		FVector3 v2 = FVector3(mesh.Vertices[i + 2].Position.X, mesh.Vertices[i + 2].Position.Y, mesh.Vertices[i + 2].Position.Z);
		
		bound.Expand(v0);
		bound.Expand(v1);
		bound.Expand(v2);
		triangleMesh->triangles.push_back(FTriangle(v0, v1, v2));
	} // end for i

	triangleMesh->worldbbox = bound;

	return triangleMesh;
}


}
