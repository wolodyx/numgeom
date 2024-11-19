#include "gtest/gtest.h"

#include "numgeom/loadfromvtk.h"

#include "utilities.h"


TEST(LoadFromFile, Vtk)
{
    TriMesh::Ptr mesh = LoadTriMeshFromVtk(TestData("k.vtk"));
    ASSERT_TRUE(mesh != TriMesh::Ptr());
}
