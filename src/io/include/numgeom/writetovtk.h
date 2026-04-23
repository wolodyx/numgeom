#ifndef NUMGEOM_IO_WRITETOVTK_H
#define NUMGEOM_IO_WRITETOVTK_H

#include <filesystem>

#include "numgeom/trimesh.h"

bool WriteToUnstructuredVtk(CTriMesh::Ptr,const std::filesystem::path&);

bool WriteToPolydataVtk(CTriMesh::Ptr, const std::filesystem::path&);

#endif  // !NUMGEOM_IO_WRITETOVTK_H
