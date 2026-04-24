#include "numgeom/loadusingocc.h"

#include <algorithm>

#include "BRepTools.hxx"
#include "RWStl.hxx"
#include "ShapeFix_Shape.hxx"
#include "STEPCAFControl_Reader.hxx"
#include "STEPControl_Reader.hxx"
#include "XCAFApp_Application.hxx"

#include "numgeom/utilities.h"

TriMesh::Ptr LoadUsingOCC(const std::filesystem::path& filename) {
  if (!std::filesystem::exists(filename)) return TriMesh::Ptr();

  std::string ext = filename.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  TopoDS_Shape shape;
  if (ext == ".step" || ext == ".stp") {
    STEPControl_Reader reader;
    std::string str = filename.string();
    if (reader.ReadFile(str.c_str()) != IFSelect_RetDone) return TriMesh::Ptr();
    // reader.PrintCheckLoad(false, IFSelect_ListByItem);
    Standard_Integer NbRoots = reader.NbRootsForTransfer();
    Standard_Integer NbTrans = reader.TransferRoots();
    shape = reader.OneShape();

    Standard_Boolean doHealing = false;
    if (doHealing) {
      ShapeFix_Shape shapeHealer(shape);
      if (shapeHealer.Perform()) shape = shapeHealer.Shape();
    }
  } else if (ext == ".brep") {
    // BRep_Builder builder;
    // BRepTools::Read(shape, filename.string().c_str(), builder);
  }

  return ConvertToTriMesh(shape);
}

Handle(TDocStd_Document) LoadStepDocument(const std::filesystem::path& filename) {
  Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
  Handle(TDocStd_Document) document;
  app->NewDocument("MDTV-XCAF", document);

  STEPCAFControl_Reader reader;
  reader.SetColorMode(true);
  reader.SetNameMode(true);
  reader.SetLayerMode(true);

  std::u8string u8_filename = filename.u8string();
  auto occ_filename = reinterpret_cast<const char*>(u8_filename.c_str());
  IFSelect_ReturnStatus status = reader.ReadFile(occ_filename);
  if (status != IFSelect_RetDone) {
    return Handle(TDocStd_Document)();
  }

  if (!reader.Transfer(document)) {
    return Handle(TDocStd_Document)();
  }

  return document;
}

Handle(Poly_Triangulation) LoadStl(const std::filesystem::path& filename) {
  std::u8string u8_filename = filename.u8string();
  auto occ_filename = reinterpret_cast<const char*>(u8_filename.c_str());
  Handle(Poly_Triangulation) tr = RWStl::ReadFile(occ_filename);
  return tr;
}
