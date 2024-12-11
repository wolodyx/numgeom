#ifndef numgeom_numgeom_shapes_h
#define numgeom_numgeom_shapes_h

#include <array>

#include "numgeom/numgeom_export.h"
#include "numgeom/trimesh.h"


NUMGEOM_EXPORT TriMesh::Ptr MakeBox(
    const std::array<gp_Pnt,8>& corners
);


NUMGEOM_EXPORT TriMesh::Ptr MakeSphere(
    const gp_Pnt& center,
    double radius,
    size_t nSlices,
    size_t nStacks
);


NUMGEOM_EXPORT TriMesh::Ptr MakeCylinder(
    const gp_Pnt& s,
    const gp_Pnt& t,
    double radius,
    size_t nSlices
);


NUMGEOM_EXPORT TriMesh::Ptr MakeCone(
    const gp_Pnt& s,
    const gp_Pnt& t,
    double radius,
    size_t nSlices
);

#endif // !numgeom_numgeom_shapes_h
