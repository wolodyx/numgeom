#ifndef NUMGEOM_DEMO_APPQT_DOWNLOADDATA_H
#define NUMGEOM_DEMO_APPQT_DOWNLOADDATA_H

#include <filesystem>
#include <string>

bool DownloadData(const std::string& url, const std::filesystem::path& dir);

#endif // !NUMGEOM_DEMO_APPQT_DOWNLOADDATA_H
