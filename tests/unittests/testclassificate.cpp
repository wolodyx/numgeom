#include "gtest/gtest.h"

#include <Bnd_Box2d.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <gp_Pnt2d.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>

#include "numgeom/classificate.h"
#include "numgeom/utilities.h"

#include "utilities.h"


Standard_Integer RoundToInt(Standard_Real value)
{
    if(value > 0.0)
        return static_cast<Standard_Integer>(std::floor(value + 0.5));
    if(value < 0.0)
        return static_cast<Standard_Integer>(std::ceil(value - 0.5));
    return 0;
}


//! Подбор такого количества разбиений на xN и yN отрезков сторон
//! коробки, при котором примерно получается nbCells ячеек.
void GetMeshSideSizes(
    const Bnd_Box2d box,
    Standard_Integer nbCells,
    Standard_Integer& xN,
    Standard_Integer& yN
)
{
    Standard_Real xMin, yMin, xMax, yMax;
    box.Get(xMin, yMin, xMax, yMax);
    Standard_Real xWidth = xMax - xMin;
    Standard_Real yWidth = yMax - yMin;
    Standard_Real maxWidth = std::max(xWidth, yWidth);
    Standard_Real minWidth = std::min(xWidth, yWidth);
    Standard_Real ratio = maxWidth / minWidth;

    Standard_Integer n1Min = 5;
    Standard_Integer n1Max = 2 * nbCells / n1Min;
    Standard_Integer n1, n2;
    for(Standard_Integer iter = 0; iter < 16; ++iter)
    {
        n1 = (n1Min + n1Max) / 2;
        Standard_Real h1 = maxWidth / n1;
        n2 = RoundToInt(minWidth / h1);
        Standard_Integer n = n1 * n2;

        if(n == nbCells || n1Max - n1Min < 2)
            break;

        if(n > nbCells)
            n1Max = n1;
        else if(n < nbCells)
            n1Min = n1;
    }

    if(xWidth != maxWidth)
        std::swap(n1, n2);
    xN = n1, yN = n2;
}


TEST(Classificate, Example)
{
    TopoDS_Shape model = LoadFromFile(TestData("rounded-cube.step"));
    ASSERT_FALSE(model.IsNull());
    const AuxClassificateData* aux = nullptr;
    for(TopExp_Explorer it(model,TopAbs_FACE); it.More(); it.Next())
    {
        TopoDS_Face face = TopoDS::Face(it.Current());
        Bnd_Box2d pCurvesBox = ComputePCurvesBox(face);
        Standard_Integer iN, jN;
        GetMeshSideSizes(pCurvesBox, 1000, iN, jN);
        Standard_Real xMin, yMin, xMax, yMax;
        pCurvesBox.Get(xMin, yMin, xMax, yMax);
        for(Standard_Integer j = 0; j <= jN; ++j)
        {
            Standard_Real y = yMin + (yMax - yMin) / jN * j;
            for(Standard_Integer i = 0; i <= iN; ++i)
            {
                Standard_Real x = xMin + (xMax - xMin) / iN * i;
                TopAbs_State state = Classificate(face, gp_Pnt2d(x,y), &aux);
                ASSERT_TRUE(state != TopAbs_UNKNOWN);
                BRepClass_FaceClassifier classifier(face, gp_Pnt2d(x,y), 1.e-7);
                TopAbs_State state2 = classifier.State();
                ASSERT_EQ(state, state2);
            }
        }
        delete aux; aux = nullptr;
    }
}
