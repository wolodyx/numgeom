#ifndef NUMGEOM_UNITTESTS_UTILITIES_H
#define NUMGEOM_UNITTESTS_UTILITIES_H

#include <filesystem>

#include <TopoDS_Shape.hxx>


std::string GetTestName();

TopoDS_Shape LoadFromFile(const std::filesystem::path&);

std::filesystem::path TestData(const std::string& fileName);

#endif // !NUMGEOM_UNITTESTS_UTILITIES_H
