#ifndef numgeom_numgeom_utilities_h
#define numgeom_numgeom_utilities_h

#include <Bnd_Box2d.hxx>

class TopoDS_Face;


Bnd_Box2d ComputePCurvesBox(const TopoDS_Face&);

#endif //! numgeom_numgeom_utilities_h
