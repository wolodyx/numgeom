#include "workbench_occ.h"

#include <format>

#include "Poly_Triangle.hxx"
#include "TDataXtd_Triangulation.hxx"
#include "TDF_ChildIterator.hxx"
#include "TDocStd_Document.hxx"
#include "XCAFApp_Application.hxx"
#include "XCAFDoc_DocumentTool.hxx"
#include "XCAFDoc_ShapeTool.hxx"

#include "numgeom/drawable_occshape.h"
#include "numgeom/loadusingocc.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject_tdocstd_document.h"

REGISTER_WORKBENCH("OCC", Workbench_OCC);

Workbench_OCC::Workbench_OCC() {}

Workbench_OCC::~Workbench_OCC() {}

std::string Workbench_OCC::GetName() const { return "OCC"; }

void Workbench_OCC::Setup() {}

void Workbench_OCC::Cleanup() {}

namespace {
std::string GetExtension(const std::filesystem::path& filename) {
  std::string ext = filename.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return ext;
}

bool IsStepFile(const std::filesystem::path& filename) {
  auto ext = GetExtension(filename);
  return ext == ".step" || ext == ".stp";
}

bool IsStlFile(const std::filesystem::path& filename) {
  auto ext = GetExtension(filename);
  return ext == ".stl";
}

bool IsIgesFile(const std::filesystem::path& filename) {
  auto ext = GetExtension(filename);
  return ext == ".iges" || ext == ".igs";
}

bool IsObjFile(const std::filesystem::path& filename) {
  auto ext = GetExtension(filename);
  return ext == ".obj";
}

bool IsVrmlFile(const std::filesystem::path& filename) {
  auto ext = GetExtension(filename);
  return ext == ".wrl";
}

bool IsGltfFile(const std::filesystem::path& filename) {
  auto ext = GetExtension(filename);
  return ext == ".glb";
}

void Append(Handle(TDocStd_Document)& main_doc,
            const Handle(TDocStd_Document)& add_doc) {
  if (main_doc.IsNull()) {
    main_doc = add_doc;
    return;
  }
  if (add_doc.IsNull())
    return;

  if (main_doc == add_doc)
    return;

  auto main_shape_tool = XCAFDoc_DocumentTool::ShapeTool(main_doc->Main());
  auto add_shape_tool = XCAFDoc_DocumentTool::ShapeTool(add_doc->Main());

  TDF_LabelSequence free_shapes;
  add_shape_tool->GetFreeShapes(free_shapes);
  for (const TDF_Label& label : free_shapes) {
    TopoDS_Shape shape;
    if (!add_shape_tool->GetShape(label, shape))
      continue;
    main_shape_tool->AddShape(shape);
  }
}

void Append(Handle(TDocStd_Document)& main_doc,
            const Handle(Poly_Triangulation)& add_trng) {
  if (add_trng.IsNull())
    return;

  // If the main document is not initialized, create an empty structure first.
  if (main_doc.IsNull()) {
    Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
    app->NewDocument("MDTV-XCAF", main_doc);
  }

  // Add the triangulation into the document hierarchy as a TDataXtd_Triangulation
  // attribute attached to a new child label of the document root.
  TDF_Label label = main_doc->Main().NewChild();
  TDataXtd_Triangulation::Set(label, add_trng);
}
}

bool Workbench_OCC::LoadFromFile(const std::filesystem::path& file_path) {
  Scene* scene = this->GetScene();

  Handle(Standard_Transient) loaded_object;
  if (IsStepFile(file_path)) {
    loaded_object = LoadStepDocument(file_path);
  } else if (IsStlFile(file_path)) {
    loaded_object = LoadStl(file_path);
  } else if (IsIgesFile(file_path)) {
    loaded_object = LoadIges(file_path);
  } else if (IsObjFile(file_path)) {
    loaded_object = LoadObj(file_path);
#ifdef NUMGEOM_OCC_LOAD_VRML
  } else if (IsVrmlFile(file_path)) {
    loaded_object = LoadVrml(file_path);
#endif
  } else if (IsGltfFile(file_path)) {
    loaded_object = LoadGltf(file_path);
  } else {
    std::cerr << std::format("Не поддерживается загрузка файла '{}'",
                             file_path.string());
    return false;
  }
  if (!loaded_object) {
    std::cerr << std::format("Ошибка при загрузке файла '{}'",
                             file_path.string());
    return false;
  }

  if (loaded_object->IsKind(STANDARD_TYPE(TDocStd_Document))) {
    auto next_doc = Handle(TDocStd_Document)::DownCast(loaded_object);
    Append(document_, next_doc);
  } else if (loaded_object->IsKind(STANDARD_TYPE(Poly_Triangulation))) {
    auto trng = Handle(Poly_Triangulation)::DownCast(loaded_object);
    Append(document_, trng);
  } else {
    std::cerr << std::format("Неизвестный тип объекта.");
  }

  this->Update();
  scene->FitScene();
  return true;
}

bool Workbench_OCC::Move(Drawable* drawable, const glm::vec3& dir) {
  SceneObject* so = drawable->GetParent();
  if (this->GetScene() != so->GetScene())
    return false;
  // Extract `TopoDS_Shape` from `drawable`.
  TopoDS_Shape shape;
  if (shape.IsNull())
    return false;
  return true;
}

void Workbench_OCC::Update() {
  Scene* scene = this->GetScene();
  if (!scene)
    return;
  if (!so_ && !document_.IsNull()) {
    so_ = scene->AddObject<SceneObject_TDocStd_Document>(document_);
  } else if (so_) {
    so_->Synchronize();
  }
}

namespace {
void ClearDocument(Handle(TDocStd_Document)& doc) {
  if (doc.IsNull())
    return;

  TDF_Label root = doc->Main();
  if (root.IsNull())
    return;

  // Remove every free shape while preserving the shape tool attribute.
  auto shape_tool = XCAFDoc_DocumentTool::ShapeTool(root);
  TDF_LabelSequence free_shapes;
  shape_tool->GetFreeShapes(free_shapes);
  for (const TDF_Label& label : free_shapes) {
    shape_tool->RemoveShape(label);
  }

  // Remove triangulation attributes attached directly under the document root
  // (see Append for Poly_Triangulation). XCAF tool attributes stay untouched.
  TDF_ChildIterator child_it(root);
  for (; child_it.More(); child_it.Next()) {
    TDF_Label label = child_it.Value();
    Handle(TDataXtd_Triangulation) attr;
    if (label.FindAttribute(TDataXtd_Triangulation::GetID(), attr))
      label.ForgetAllAttributes(true);
  }
}
}

void Workbench_OCC::Clear() {
  ClearDocument(document_);
  this->Update();
}

#define REGISTER_COMMAND(WorkbenchType, Cmd, CmdId) \
namespace { \
  class Register_##WorkbenchType_Cmd { \
   public: \
    Register_##WorkbenchType_Cmd() { \
      auto create_function = [](Workbench* wb) { return new Cmd(wb); }; \
      bool ret = Workbench::RegisterCommand(#WorkbenchType, CmdId, create_function); \
      if (!ret) std::abort(); \
    } \
  }; \
  Register_##WorkbenchType_Cmd register_##WorkbenchType_cmd; \
}

#include "command_changevisibility.h"
REGISTER_COMMAND(OCC, Command_ChangeVisibility, "change-visibility");
