#ifndef examples_appqt_loadtotrimesh_h
#define examples_appqt_loadtotrimesh_h

#include "numgeom/trimesh.h"

TriMesh::Ptr LoadToTriMesh(const std::filesystem::path& filename);

#endif  // !numgeom_io_loadtotrimesh_h
