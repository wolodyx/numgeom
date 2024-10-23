#include "writevtk.h"

#include <fstream>


struct Dumper::Internal
{
    std::vector<gp_Pnt2d> points;
    std::list<std::vector<gp_Pnt2d>> polylines;
};


Dumper::Dumper()
{
    pimpl = new Internal;
}


Dumper::~Dumper()
{
    delete pimpl;
}


Dumper& Dumper::AddPoints(const std::vector<gp_Pnt2d>& points)
{
    pimpl->points.insert(pimpl->points.end(), points.begin(), points.end());
    return *this;
}


Dumper& Dumper::AddPolyline(const std::vector<gp_Pnt2d>& polyline)
{
    pimpl->polylines.push_back(polyline);
    return *this;
}


Standard_Boolean Dumper::Save(const std::filesystem::path& fileName) const
{
    std::ofstream file(fileName);
    if(!file.is_open())
        return Standard_False;

    size_t nbPoints = pimpl->points.size();
    for(const auto& pl : pimpl->polylines)
        nbPoints += pl.size();

    file << "# vtk DataFile Version 3.0" << std::endl;
    file << "numgeom output" << std::endl;
    file << "ASCII" << std::endl;
    file << "DATASET UNSTRUCTURED_GRID" << std::endl;
    file << "POINTS " << nbPoints << " double" << std::endl;

    for(const gp_Pnt2d& pt : pimpl->points)
        file << pt.X() << ' ' << pt.Y() << " 0.0 ";
    file << std::endl;
    for(const auto& pl : pimpl->polylines)
    {
        for(const auto& pt : pl)
            file << pt.X() << ' ' << pt.Y() << " 0.0 ";
        file << std::endl;
    }

    size_t nbVerts = pimpl->points.size();
    size_t nbLines = 0;
    for(const auto& pl : pimpl->polylines)
        nbLines += (pl.size() - 1);

    file << "CELLS " << nbVerts + nbLines << ' ' << 2 * nbVerts + 3 * nbLines << std::endl;
    for(size_t i = 0; i < nbVerts; ++i)
        file << " 1 " << i;
    file << std::endl;
    size_t index = nbVerts;
    for(const auto& pl : pimpl->polylines)
    {
        for(size_t i = 0; i < pl.size() - 1; ++i, ++index)
            file << "2 " << index << ' ' << index + 1 << ' ';
        ++index;
    }
    file << std::endl;

    file << "CELL_TYPES " << nbVerts + nbLines << std::endl;
    for(size_t i = 0; i < nbVerts; ++i)
        file << "1 ";
    file << std::endl;
    for(size_t i = 0; i < nbLines; ++i)
        file << "3 ";
    file << std::endl;

    return Standard_True;
}
