#ifndef NUMGEOM_IO_WRITETOVTK_H
#define NUMGEOM_IO_WRITETOVTK_H

#include <filesystem>

#include "numgeom/trimesh.h"


bool WriteToVtk(CTriMesh::Ptr, const std::filesystem::path& filename);

#endif // !NUMGEOM_IO_WRITETOVTK_H
