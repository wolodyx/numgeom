#ifndef numgeom_io_loadtotrimesh_h
#define numgeom_io_loadtotrimesh_h

#include "numgeom/trimesh.h"


TriMesh::Ptr LoadToTriMesh(const std::filesystem::path& filename);

#endif // !numgeom_io_loadtotrimesh_h
