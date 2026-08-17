#ifndef numgeom_io_loadusingocc_h
#define numgeom_io_loadusingocc_h

#include <filesystem>

#include "Standard_Handle.hxx"

#include "numgeom/occ_export.h"
#include "numgeom/trimesh.h"

class Poly_Triangulation;
class TDocStd_Document;

OCC_EXPORT TriMesh::Ptr LoadUsingOCC(const std::filesystem::path&);

OCC_EXPORT Handle(TDocStd_Document) LoadStepDocument(const std::filesystem::path&);

OCC_EXPORT Handle(TDocStd_Document) LoadIges(const std::filesystem::path&);

#ifdef NUMGEOM_OCC_LOAD_VRML
OCC_EXPORT Handle(TDocStd_Document) LoadVrml(const std::filesystem::path&);
#endif

OCC_EXPORT Handle(TDocStd_Document) LoadGltf(const std::filesystem::path&);

OCC_EXPORT Handle(Poly_Triangulation) LoadStl(const std::filesystem::path&);

OCC_EXPORT Handle(Poly_Triangulation) LoadObj(const std::filesystem::path&);

#endif  // !numgeom_io_loadusingocc_h
