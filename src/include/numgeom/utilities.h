#ifndef numgeom_numgeom_utilities_h
#define numgeom_numgeom_utilities_h

#include <filesystem>

#include <Bnd_Box.hxx>
#include <Bnd_Box2d.hxx>

class Geom_BoundedCurve;
class Geom_Curve;
class Geom_Surface;
class Geom2d_BoundedCurve;
class Geom2d_Curve;
class TopoDS_Edge;
class TopoDS_Face;
class TopoDS_Shape;


Bnd_Box2d ComputePCurvesBox(const TopoDS_Face&);

Bnd_Box ComputeBoundBox(const TopoDS_Shape&);

Standard_Boolean IsClosed(const TopoDS_Edge&);

Standard_Boolean IsWaterproof(const TopoDS_Shape&);


Standard_Boolean OnFace(
    const TopoDS_Face&,
    const gp_Pnt2d&
);


Standard_Real Project(
    const gp_Pnt2d&,
    const Handle(Geom2d_Curve)&
);


Standard_Real Project(
    const gp_Pnt&,
    const Handle(Geom_Curve)&
);


gp_Pnt2d Project(
    const gp_Pnt&,
    const Handle(Geom_Surface)&
);


Handle(Geom2d_BoundedCurve) Project(
    const Handle(Geom_BoundedCurve)&,
    const Handle(Geom_Surface)&
);


bool ReadFromFile(
    const std::filesystem::path&,
    TopoDS_Shape&
);


bool WriteToStep(
    const TopoDS_Shape&,
    const std::filesystem::path&
);

#endif //! numgeom_numgeom_utilities_h
