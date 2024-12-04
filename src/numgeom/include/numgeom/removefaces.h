#ifndef numgeom_numgeom_algoremovefaces_h
#define numgeom_numgeom_algoremovefaces_h

#include <TopTools_ListOfShape.hxx>

class TopoDS_Shape;


TopoDS_Shape RemoveFaces(
    const TopoDS_Shape& initShape,
    const TopTools_ListOfShape& facesToRemove
);

#endif // !numgeom_numgeom_algoremovefaces_h
