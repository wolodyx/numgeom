#ifndef numgeom_numgeom_utilities_h
#define numgeom_numgeom_utilities_h

#include <Bnd_Box.hxx>
#include <Bnd_Box2d.hxx>

class TopoDS_Face;
class TopoDS_Shape;


Bnd_Box2d ComputePCurvesBox(const TopoDS_Face&);

Bnd_Box ComputeBoundBox(const TopoDS_Shape&);

#endif //! numgeom_numgeom_utilities_h
