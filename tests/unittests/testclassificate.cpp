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


TEST(Classificate, Face)
{
    TopoDS_Shape model = LoadFromFile(TestData("rounded-cube.step"));
    ASSERT_FALSE(model.IsNull());
    const AuxClassificateData2d* aux = nullptr;
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
