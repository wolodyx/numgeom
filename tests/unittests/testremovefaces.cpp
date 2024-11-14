#include "gtest/gtest.h"

#include <filesystem>

#include <TopExp.hxx>

#include "numgeom/removefaces.h"
#include "numgeom/utilities.h"

#include "utilities.h"

#include <BRep_Tool.hxx>
#include <Geom_Plane.hxx>
#include <TopoDS.hxx>

GTEST_DECLARE_bool_(write_output);


namespace {;
struct TestParam
{
    std::string modelName;
    std::vector<int> facesIds;

    std::filesystem::path ModelPath() const
    {
        return TestData(modelName);
    }
};

class RemoveFacesTest
    : public testing::TestWithParam<TestParam>
{
};
}

static std::vector<TestParam> s_testData = {
    {"circle-hole-in-quad.step", {3}},
    {"corner.step", {2,6}},
    {"one-side-rounded-cube.step", {3,7,9,10}},
    {"rounded-cube.step", {7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26}},
    {"handle.step", {1,8,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,30,31,33,34}},
};


INSTANTIATE_TEST_SUITE_P(Xxx, RemoveFacesTest, testing::ValuesIn(s_testData));


TEST_P(RemoveFacesTest, FromStep)
{
    const auto& param = this->GetParam();

    TopoDS_Shape shape;
    ASSERT_TRUE(ReadFromFile(param.ModelPath(),shape));
    ASSERT_EQ(shape.ShapeType(), TopAbs_SOLID);
    ASSERT_TRUE(IsWaterproof(shape));

    TopTools_ListOfShape facesToRemove;
    {
        TopTools_IndexedMapOfShape allFaces;
        TopExp::MapShapes(shape, TopAbs_FACE, allFaces);
        for(auto id : param.facesIds)
            facesToRemove.Append(allFaces.FindKey(id));
    }

    TopoDS_Shape result = RemoveFaces(shape, facesToRemove);
    ASSERT_TRUE(!result.IsNull());

    if(GTEST_FLAG_GET(write_output))
    {
        std::filesystem::path outFilename = GetTestName() + param.modelName;
        ASSERT_TRUE(WriteToStep(result,outFilename));
    }
}
