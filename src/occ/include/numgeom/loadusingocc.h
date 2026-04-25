#ifndef numgeom_io_loadusingocc_h
#define numgeom_io_loadusingocc_h

#include <filesystem>

#include "Standard_Handle.hxx"

#include "numgeom/trimesh.h"

class Poly_Triangulation;
class TDocStd_Document;

TriMesh::Ptr LoadUsingOCC(const std::filesystem::path&);

Handle(TDocStd_Document) LoadStepDocument(const std::filesystem::path&);

Handle(TDocStd_Document) LoadIges(const std::filesystem::path&);

#ifdef NUMGEOM_OCC_LOAD_VRML
Handle(TDocStd_Document) LoadVrml(const std::filesystem::path&);
#endif

Handle(TDocStd_Document) LoadGltf(const std::filesystem::path&);

Handle(Poly_Triangulation) LoadStl(const std::filesystem::path&);

Handle(Poly_Triangulation) LoadObj(const std::filesystem::path&);

#endif  // !numgeom_io_loadusingocc_h
