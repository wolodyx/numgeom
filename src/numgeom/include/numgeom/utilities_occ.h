#ifndef NUMGEOM_NUMGEOM_UTILITIES_OCC_H
#define NUMGEOM_NUMGEOM_UTILITIES_OCC_H

#include <filesystem>

#include "TopoDS_Shape.hxx"


TopoDS_Shape LoadFromFile(const std::filesystem::path&);

#endif // !NUMGEOM_NUMGEOM_UTILITIES_OCC_H
