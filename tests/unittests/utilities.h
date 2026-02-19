#ifndef NUMGEOM_UNITTESTS_UTILITIES_H
#define NUMGEOM_UNITTESTS_UTILITIES_H

#include <filesystem>


std::string GetTestName();

std::filesystem::path TestData(const std::string& fileName);

#endif // !NUMGEOM_UNITTESTS_UTILITIES_H
