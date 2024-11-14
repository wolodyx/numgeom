#ifndef numgeom_numgeom_ffextintersections_h
#define numgeom_numgeom_ffextintersections_h

#include <list>
#include <map>

#include <Standard_Handle.hxx>
#include <TopoDS_Face.hxx>
#include <TopTools_MapOfShape.hxx>

class Geom_BoundedCurve;
class Geom_Curve;
class Geom_Surface;
class Geom2d_BoundedCurve;


/**\class FaceFaceIntersections
\brief Вспомогательное средство для вычисления пересечений расширенных граней
       друг с другом и кривой с расширенными гранями.
*/
class FaceFaceIntersections
{
public:

    struct IntFF
    {
        TopoDS_Face F;
        Handle(Geom_Curve) C;
    };

    struct IntCF
    {
        TopoDS_Face F; //< Грань расширенной поверхности.
        Standard_Real t; //< Параметр точки пересечения кривой с расширенной поверхностью грани `F`.
    };

    class FFResult
    {
        const std::list<IntFF>& myIntersections;
    public:
        FFResult(const std::list<IntFF>& ints);
        auto begin() const { return myIntersections.begin(); }
        auto end() const   { return myIntersections.end(); }
    };

    class CFResult
    {
        const std::list<IntCF>& myIntersections;
    public:
        CFResult(const std::list<IntCF>& ints);
        auto begin() const { return myIntersections.begin(); }
        auto end() const   { return myIntersections.end(); }
    };

public:

    //! Конструктор по множеству граней, которые будут участвовать в пересечениях.
    FaceFaceIntersections(const TopTools_MapOfShape& externalFaces);


    //! Пересечения грани со множеством граней.
    FFResult GetIntersections(const TopoDS_Face&) const;


    //! Пересечения кривой со множеством граней.
    CFResult GetIntersections(const Handle(Geom_Curve)& curve) const;


private:

    //! Расширенные поверхности граней.
    std::unordered_map<TopoDS_Face, Handle(Geom_Surface)> myFace2extendedSurface;

    //! Кешированный результат пересечения граней с гранями.
    mutable std::unordered_map<TopoDS_Face, std::list<IntFF>> myFace2intersections;
    mutable std::unordered_map<Handle(Geom_Curve), std::list<IntCF>> myCurve2intersections;
};


Standard_Boolean IsIntersected(
    const Handle(Geom_BoundedCurve)& curve,
    const TopTools_MapOfShape& faces
);


Standard_Boolean IsIntersected(
    const Handle(Geom2d_BoundedCurve)& curve,
    const TopoDS_Face& face
);
#endif // !numgeom_numgeom_ffextintersections_h
