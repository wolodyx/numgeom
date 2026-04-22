#ifndef numgeom_io_loadusingocc_h
#define numgeom_io_loadusingocc_h

#include <filesystem>

#include "Standard_Handle.hxx"

#include "numgeom/trimesh.h"

class TDocStd_Document;

TriMesh::Ptr LoadUsingOCC(const std::filesystem::path&);

Handle(TDocStd_Document) LoadStepDocument(const std::filesystem::path&);
#endif  // !numgeom_io_loadusingocc_h
