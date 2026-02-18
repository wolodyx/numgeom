#ifndef numgeom_numgeom_shapes_h
#define numgeom_numgeom_shapes_h

#include <array>

#include "numgeom/numgeom_export.h"
#include "numgeom/trimesh.h"


NUMGEOM_EXPORT TriMesh::Ptr MakeBox(
    const std::array<TriMesh::NodeType,8>& corners
);


NUMGEOM_EXPORT TriMesh::Ptr MakeSphere(
    const TriMesh::NodeType& center,
    double radius,
    size_t nSlices,
    size_t nStacks
);


NUMGEOM_EXPORT TriMesh::Ptr MakeCylinder(
    const TriMesh::NodeType& s,
    const TriMesh::NodeType& t,
    double radius,
    size_t nSlices
);


NUMGEOM_EXPORT TriMesh::Ptr MakeCone(
    const TriMesh::NodeType& s,
    const TriMesh::NodeType& t,
    double radius,
    size_t nSlices
);

#endif // !numgeom_numgeom_shapes_h
