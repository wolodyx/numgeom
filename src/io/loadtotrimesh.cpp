#include "numgeom/loadtotrimesh.h"

#include <algorithm>

#include "numgeom/loadfromvtk.h"
#include "numgeom/loadusingocc.h"


TriMesh::Ptr LoadToTriMesh(const std::filesystem::path& filename)
{
    if(!std::filesystem::exists(filename))
        return TriMesh::Ptr();

    std::string ext = filename.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c){ return std::tolower(c); });

    if(ext == ".step" || ext == ".stp")
        return LoadUsingOCC(filename);

    if(ext == ".vtk")
        return LoadTriMeshFromVtk(filename);

    return TriMesh::Ptr();
}
