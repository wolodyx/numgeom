#ifndef numgeom_numgeom_classificate_h
#define numgeom_numgeom_classificate_h

#include <Standard_TypeDef.hxx>
#include <TopAbs_State.hxx>

#include "numgeom/numgeom_export.h"

class AuxClassificateData;
class gp_Pnt2d;
class TopoDS_Face;


/**\brief Классификация точки относительно параметрического пространства грани.
\param face Топологическая грань, из которой извлекается двумерная область в
             параметрическом пространстве.
\param point Классифицируемая точка.
\param aux Вспомогательный объект, ускоряющий вычисления при повторном вызове
           функции.
*/
NUMGEOM_EXPORT TopAbs_State Classificate(
    const TopoDS_Face& face,
    const gp_Pnt2d& point,
    const AuxClassificateData** aux = nullptr
);

#endif // !numgeom_numgeom_classificate_h
