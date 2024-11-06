#include "numgeom/trimesh.h"


TriMesh::TriMesh()
{
}


TriMesh::TriMesh(Standard_Integer nbNodes, Standard_Integer nbCells)
    : myNodes(nbNodes), myCells(nbCells)
{
}


Standard_Integer TriMesh::NbNodes() const
{
    return myNodes.size();
}


Standard_Integer TriMesh::NbCells() const
{
    return myCells.size();
}


const gp_Pnt& TriMesh::GetNode(Standard_Integer index) const
{
    return myNodes[index];
}


const TriMesh::Cell& TriMesh::GetCell(Standard_Integer index) const
{
    return myCells[index];
}


gp_Pnt& TriMesh::GetNode(Standard_Integer index)
{
    return myNodes[index];
}


TriMesh::Cell& TriMesh::GetCell(Standard_Integer index)
{
    return myCells[index];
}


TriMesh::Ptr TriMesh::Create(Standard_Integer nbNodes, Standard_Integer nbCells)
{
    if(nbNodes == 0 || nbCells == 0)
        return Ptr();

    return Ptr(new TriMesh(nbNodes,nbCells));
}


Standard_Boolean TriMesh::Dump(
    const std::filesystem::path& fileName
) const
{
    std::ofstream file(fileName);
    if(!file.is_open())
        return Standard_False;

    file << "# vtk DataFile Version 3.0" << std::endl;
    file << "numgeom output" << std::endl;
    file << "ASCII" << std::endl;
    file << "DATASET UNSTRUCTURED_GRID" << std::endl;

    Standard_Integer nbNodes = this->NbNodes();
    file << "POINTS " << nbNodes << " double" << std::endl;
    for(Standard_Integer i = 0; i < nbNodes; ++i)
    {
        const gp_Pnt& pt = this->GetNode(i);
        file << pt.X() << ' ' << pt.Y() << ' ' << pt.Z() << ' ';
    }
    file << std::endl;

    Standard_Integer nbCells = this->NbCells();
    file << "CELLS " << nbCells << ' ' << 4 * nbCells << std::endl;
    for(Standard_Integer i = 0; i < nbCells; ++i)
    {
        const TriMesh::Cell& cell = this->GetCell(i);
        file << "3 " << cell.na << ' ' << cell.nb << ' ' << cell.nc << ' ';
    }
    file << std::endl;

    file << "CELL_TYPES " << nbCells << std::endl;
    for(size_t i = 0; i < nbCells; ++i)
        file << "5 "; //< VTK_TRIANGLE
    file << std::endl;

    return Standard_True;
}