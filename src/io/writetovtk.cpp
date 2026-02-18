#include "numgeom/writetovtk.h"

#include <fstream>


bool WriteToVtk(CTriMesh::Ptr mesh, const std::filesystem::path& filename)
{
    std::ofstream file(filename);
    if(!file.is_open())
        return false;

    file << "# vtk DataFile Version 3.0" << std::endl;
    file << "numgeom output" << std::endl;
    file << "ASCII" << std::endl;
    file << "DATASET UNSTRUCTURED_GRID" << std::endl;

    size_t nbNodes = mesh->NbNodes();
    file << "POINTS " << nbNodes << " double" << std::endl;
    for(size_t i = 0; i < nbNodes; ++i)
    {
        const auto& pt = mesh->GetNode(i);
        file << pt.x << ' ' << pt.y << ' ' << pt.z << ' ';
    }
    file << std::endl;

    size_t nbCells = mesh->NbCells();
    file << "CELLS " << nbCells << ' ' << 4 * nbCells << std::endl;
    for(size_t i = 0; i < nbCells; ++i)
    {
        const CTriMesh::Cell& cell = mesh->GetCell(i);
        file << "3 " << cell.na << ' ' << cell.nb << ' ' << cell.nc << ' ';
    }
    file << std::endl;

    file << "CELL_TYPES " << nbCells << std::endl;
    for(size_t i = 0; i < nbCells; ++i)
        file << "5 "; //< VTK_TRIANGLE
    file << std::endl;

    return true;
}
