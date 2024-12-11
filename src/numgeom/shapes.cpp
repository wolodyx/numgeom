#include "numgeom/shapes.h"

#include <cassert>
#include <cmath>

#include <gp_Ax1.hxx>


TriMesh::Ptr MakeBox(const std::array<gp_Pnt,8>& corners)
{
    TriMesh::Ptr mesh = TriMesh::Create(8, 12);
    size_t iNode = 0;
    for(const auto& p : corners)
        mesh->GetNode(iNode++) = p;

    mesh->GetCell(0) = TriMesh::Cell(0, 2, 1);
    mesh->GetCell(1) = TriMesh::Cell(0, 3, 2);
    mesh->GetCell(2) = TriMesh::Cell(4, 5, 6);
    mesh->GetCell(3) = TriMesh::Cell(4, 6, 7);
    mesh->GetCell(4) = TriMesh::Cell(0, 1, 5);
    mesh->GetCell(5) = TriMesh::Cell(0, 5, 4);
    mesh->GetCell(6) = TriMesh::Cell(3, 6, 2);
    mesh->GetCell(7) = TriMesh::Cell(3, 7, 6);
    mesh->GetCell(8) = TriMesh::Cell(0, 7, 3);
    mesh->GetCell(9) = TriMesh::Cell(0, 4, 7);
    mesh->GetCell(10)= TriMesh::Cell(1, 2, 6);
    mesh->GetCell(11)= TriMesh::Cell(1, 6, 5);

    return mesh;
}


TriMesh::Ptr MakeSphere(
    const gp_Pnt& center,
    double radius,
    size_t nSlices,
    size_t nStacks)
{
    assert(nStacks > 2 && nSlices > 3);

    size_t nNodes = 2 + (nStacks - 1) * nSlices;
    size_t nTriangles = 2 * nSlices + 2 * (nStacks - 2) * nSlices;
    TriMesh::Ptr mesh = TriMesh::Create(nNodes, nTriangles);

    size_t iNode = 0;
    // Add point of (u,v) = (-pi/2,0).
    mesh->GetNode(iNode++) = center.Translated(gp_Vec(0,0,-radius));
    for(size_t iStack = 0; iStack < nStacks - 1; ++iStack)
    {
        double u = (iStack + 1) * M_PI / nStacks - M_PI_2;
        double cos_u = std::cos(u);
        double sin_u = std::sin(u);

        for(size_t iSlice = 0; iSlice < nSlices; ++iSlice)
        {
            double v = 2.0 * M_PI / nSlices * iSlice;
            double cos_v = std::cos(v);
            double sin_v = std::sin(v);

            gp_Vec p(
                radius * cos_u * cos_v,
                radius * cos_u * sin_v,
                radius * sin_u);
            mesh->GetNode(iNode++) = center.Translated(p);
        }
    }
    // Add point of (u,v) = (pi/2,0).
    mesh->GetNode(iNode++) = center.Translated(gp_Vec(0,0,+radius));
    assert(iNode == nNodes);

    size_t iTria = 0;

    // Triangles at the south pole.
    for(size_t i = 0; i < nSlices; ++i)
        mesh->GetCell(iTria++) = TriMesh::Cell(0, 1 + (i + 1) % nSlices, i + 1);

    // Triangles at the internal stacks.
    for(size_t iStack = 1; iStack < nStacks - 1; ++iStack)
    {
        size_t downStartNode = 1 + (iStack - 1) * nSlices;
        size_t upStartNode = 1 + iStack * nSlices;
        for(size_t iSlice = 0; iSlice < nSlices; ++iSlice)
        {
            size_t p00 = downStartNode + iSlice;
            size_t p01 = downStartNode + (iSlice + 1) % nSlices;
            size_t p10 = upStartNode + iSlice;
            size_t p11 = upStartNode + (iSlice + 1) % nSlices;
            mesh->GetCell(iTria++) = TriMesh::Cell(p00, p01, p11);
            mesh->GetCell(iTria++) = TriMesh::Cell(p00, p11, p10);
        }
    }

    // Triangles at the north pole.
    size_t poleNode = nNodes - 1;
    size_t stackStartNode = nNodes - 1 - nSlices;
    for(size_t i = 0; i < nSlices; ++i)
        mesh->GetCell(iTria++) = TriMesh::Cell(poleNode, stackStartNode + i, stackStartNode + (i+1)%nSlices);

    assert(iTria == nTriangles);

    return mesh;
}


namespace {;

gp_Dir OrthoDir(const gp_Dir& dir)
{
    dir.X(), dir.Y(), dir.Z();
    Standard_Integer iMinComp = 0;
    if( std::abs(dir.Y()) < std::abs(dir.X())
     && std::abs(dir.Y()) < std::abs(dir.Z()) )
        iMinComp = 1;
    else if( std::abs(dir.Z()) < std::abs(dir.X())
          && std::abs(dir.Z()) < std::abs(dir.Y()))
        iMinComp = 2;

    gp_XYZ ortho = dir.XYZ();
    ortho.SetCoord(iMinComp, 0.0);
    Standard_Integer iNextComp1 = 1, iNextComp2 = 2;
    if(iMinComp == 1)
        iNextComp1 = 2, iNextComp2 = 0;
    else if(iMinComp == 2)
        iNextComp1 = 0, iNextComp2 = 1;
    Standard_Real& c1 = ortho.ChangeCoord(iNextComp1);
    Standard_Real& c2 = ortho.ChangeCoord(iNextComp2);
    std::swap(c1, c2);
    c1 = -c1;
    return ortho;
}


gp_Trsf MakeTransformFromZAxis(
    const gp_Pnt& s,
    const gp_Pnt& t
)
{
    gp_Trsf tr;
    tr.SetTranslation(gp_Vec(s.Coord()));
    gp_Dir dir1(0, 0, 1);
    gp_Dir dir2(gp_Vec(s,t));
    Standard_Real angle = dir1.Angle(dir2);
    gp_Dir n;
    if(angle < M_PI/180.0)
        n = dir1.Crossed(dir2);
    else
        n = OrthoDir(dir2);
    gp_Ax1 ax(s, n);
    tr.SetRotation(ax, angle);
    return tr;
}
}


TriMesh::Ptr MakeCylinder(
    const gp_Pnt& s,
    const gp_Pnt& t,
    double radius,
    size_t nSlices)
{
    double height = s.Distance(t);

    size_t nNodes = 2 * nSlices;
    size_t nTrias = 2 * nSlices;

    // Build a vertically standing cylinder of a given height.
    TriMesh::Ptr mesh = TriMesh::Create(nNodes, nTrias);
    for(size_t iSlice = 0; iSlice < nSlices; ++iSlice)
    {
        double t = iSlice * 2 * M_PI / nSlices;
        double x = radius * std::cos(t);
        double y = radius * std::sin(t);
        mesh->GetNode(iSlice) = gp_Pnt(x, y, 0.0);
        mesh->GetNode(iSlice + nSlices) = gp_Pnt(x, y, height);
    }

    size_t iTria = 0;
    for(size_t iSlice = 0; iSlice < nSlices; ++iSlice)
    {
        size_t p00 = iSlice;
        size_t p01 = (iSlice + 1) % nSlices;
        size_t p10 = nSlices + iSlice;
        size_t p11 = nSlices + (iSlice + 1) % nSlices;
        mesh->GetCell(iTria++) = TriMesh::Cell(p00, p01, p11);
        mesh->GetCell(iTria++) = TriMesh::Cell(p00, p11, p10);
    }

    gp_Trsf tr = MakeTransformFromZAxis(s, t);
    mesh->Transform(tr);

    return mesh;
}


TriMesh::Ptr MakeCone(
    const gp_Pnt& s,
    const gp_Pnt& t,
    double radius,
    size_t nSlices)
{
    size_t nNodes = 1 + nSlices;
    size_t nTrias = nSlices;
    TriMesh::Ptr mesh = TriMesh::Create(nNodes, nTrias);
    size_t iNode = 0;
    mesh->GetNode(iNode++) = gp_Pnt(0,0,0); //< We'll update it later.
    for(size_t iSlice = 0; iSlice < nSlices; ++iSlice)
    {
        double t = iSlice * 2 * M_PI / nSlices;
        double x = radius * std::cos(t);
        double y = radius * std::sin(t);
        mesh->GetNode(iNode++) = gp_Pnt(x, y, 0);
    }

    gp_Trsf tr = MakeTransformFromZAxis(s, t);
    mesh->Transform(tr);
    mesh->GetNode(0) = t;

    for(size_t iSlice = 0; iSlice < nSlices; ++iSlice)
        mesh->GetCell(iSlice) = TriMesh::Cell(0, iSlice+1, 1 + (iSlice + 1) % nSlices);

    return mesh;
}
