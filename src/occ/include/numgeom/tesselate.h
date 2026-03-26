#ifndef numgeom_numgeom_tesselate_h
#define numgeom_numgeom_tesselate_h

#include <Standard_Handle.hxx>
#include <gp_Pnt2d.hxx>
#include <vector>

#include "numgeom/occ_export.h"

class Geom_Curve;
class Geom2d_Curve;
class TopoDS_Face;
class TopoDS_Wire;

OCC_EXPORT void Tesselate(const Handle(Geom_Curve) & C,
                              Standard_Real uFirst, Standard_Real uLast,
                              std::vector<gp_Pnt>& polyline,
                              Standard_Real relAccuracy = 0.01);

OCC_EXPORT void Tesselate(const Handle(Geom2d_Curve) & C,
                              Standard_Real uFirst, Standard_Real uLast,
                              std::vector<gp_Pnt2d>& polyline,
                              Standard_Real relAccuracy = 0.01);

OCC_EXPORT void Tesselate(const TopoDS_Face& F, const TopoDS_Wire& W,
                              std::vector<gp_Pnt2d>& polyline,
                              Standard_Real relAccuracy = 0.01);

#endif  // !numgeom_numgeom_tesselate_h
