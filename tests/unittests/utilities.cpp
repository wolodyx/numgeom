#include "utilities.h"

#include <gtest/gtest.h>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <ShapeFix_Shape.hxx>
#include <STEPControl_Reader.hxx>

#include "configure.h"


namespace {;

//! ���� � ��������� �������� � ��������� �������.
std::filesystem::path GetTestDataDir()
{
    return TEST_DATA_DIR;
}

std::filesystem::path GetExternalTestDataDir()
{
    using namespace std::filesystem;
#ifdef EXTERNAL_TEST_DATA_DIR
    return EXTERNAL_TEST_DATA_DIR;
#else
    return path();
#endif
}
}


std::filesystem::path TestData(const std::string& fileName)
{
    return GetTestDataDir()/fileName;
}


TopoDS_Shape LoadFromFile(const std::filesystem::path& fileName)
{
    if(!std::filesystem::exists(fileName))
        return TopoDS_Shape();

    std::string ext = fileName.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c){ return std::tolower(c); });
    TopoDS_Shape shape;
    if(ext == ".step" || ext == ".stp")
    {
        STEPControl_Reader reader;
        std::string str = fileName.u8string();
        if(reader.ReadFile(str.c_str()) != IFSelect_RetDone)
            return TopoDS_Shape();
        //reader.PrintCheckLoad(false, IFSelect_ListByItem);
        Standard_Integer NbRoots = reader.NbRootsForTransfer();
        Standard_Integer NbTrans = reader.TransferRoots();
        shape = reader.OneShape();

        Standard_Boolean doHealing = false;
        if(doHealing)
        {
            ShapeFix_Shape shapeHealer(shape);
            if(shapeHealer.Perform())
                shape = shapeHealer.Shape();
        }
    } else if(ext == ".brep")
    {
        BRep_Builder builder;
        BRepTools::Read(shape, fileName.string().c_str(), builder);
    }

    return shape;
}


std::string GetTestName()
{
    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testName = test_info->name();
    std::replace(testName.begin(), testName.end(), '/', '_');
    return std::string(test_info->test_suite_name()) + '-' + testName;
}
