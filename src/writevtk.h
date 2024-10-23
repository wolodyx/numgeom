#ifndef NUMGEOM_OCC_WRITEVTK_H
#define NUMGEOM_OCC_WRITEVTK_H

#include <filesystem>
#include <vector>

#include <Standard_TypeDef.hxx>
#include <gp_Pnt2d.hxx>


class Dumper
{
public:

    Dumper();
    ~Dumper();

    Dumper& AddPoints(const std::vector<gp_Pnt2d>&);
    Dumper& AddPolyline(const std::vector<gp_Pnt2d>& polyline);

    Standard_Boolean Save(const std::filesystem::path&) const;

private:
    Dumper(const Dumper&) = delete;
    Dumper& operator=(const Dumper&) const = delete;

private:
    struct Internal;
    Internal* pimpl;
};

#endif // !NUMGEOM_OCC_WRITEVTK_H
