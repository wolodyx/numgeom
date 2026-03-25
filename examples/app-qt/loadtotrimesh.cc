#include "loadtotrimesh.h"

#include <algorithm>

#include "numgeom/loadfromvtk.h"
#ifdef USE_NUMGEOM_MODULE_OCC
#  include "numgeom/loadusingocc.h"
#endif

TriMesh::Ptr LoadToTriMesh(const std::filesystem::path& filename) {
  if (!std::filesystem::exists(filename)) return TriMesh::Ptr();

  std::string ext = filename.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return std::tolower(c); });

#ifdef USE_NUMGEOM_MODULE_OCC
  if (ext == ".step" || ext == ".stp") return LoadUsingOCC(filename);
#endif

  if (ext == ".vtk") return LoadTriMeshFromVtk(filename);

  return TriMesh::Ptr();
}
