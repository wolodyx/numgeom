#ifndef numgeom_io_loadusingocc_h
#define numgeom_io_loadusingocc_h

#include <filesystem>

#include "numgeom/trimesh.h"


TriMesh::Ptr LoadUsingOCC(const std::filesystem::path&);

#endif // !numgeom_io_loadusingocc_h
