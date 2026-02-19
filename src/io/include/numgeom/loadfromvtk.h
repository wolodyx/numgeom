#ifndef numgeom_numgeom_loadfromvtk_h
#define numgeom_numgeom_loadfromvtk_h

#include <filesystem>

#include "numgeom/numgeomio_export.h"
#include "numgeom/trimesh.h"


IO_EXPORT TriMesh::Ptr LoadTriMeshFromVtk(
    const std::filesystem::path&
);

#endif // !numgeom_numgeom_loadfromvtk_h
