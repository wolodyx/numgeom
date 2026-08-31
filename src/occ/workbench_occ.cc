#include "workbench_occ.h"

#include <format>

#include "BRepBuilderAPI_Transform.hxx"
#include "Poly_Triangle.hxx"
#include "TDataXtd_Triangulation.hxx"
#include "gp_GTrsf.hxx"
#include "gp_Pnt.hxx"
#include "gp_XYZ.hxx"
#include "TDF_ChildIterator.hxx"
#include "TDocStd_Document.hxx"
#include "XCAFApp_Application.hxx"
#include "XCAFDoc_DocumentTool.hxx"
#include "XCAFDoc_ShapeTool.hxx"

#include "numgeom/drawable_occshape.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject_tdocstd_document.h"

#include "loadusingocc.h"

REGISTER_WORKBENCH("OCC", Workbench_OCC);

Workbench_OCC::Workbench_OCC() {}

Workbench_OCC::~Workbench_OCC() {}

std::string Workbench_OCC::GetName() const { return "OCC"; }

void Workbench_OCC::Setup() {}

void Workbench_OCC::Cleanup() {}

namespace {
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

// Converts a column-major GLM 4x4 matrix (numgeom uses the OpenGL convention)
// into an OpenCascade general transformation.
gp_Trsf GlmToGTrsf(const glm::mat4x4& m) {
  gp_Trsf gtrsf;
  // glm::mat4x4 is indexed as [column][row].
  //for (int col = 0; col < 3; ++col) {
  //  for (int row = 0; row < 3; ++row) {
  //    gtrsf.SetValue(row + 1, col + 1, transform_matrix[col][row]);
  //  }
  //}
  //gtrsf.SetTranslationPart(gp_XYZ(transform_matrix[3][0],
  //                                transform_matrix[3][1],
  //                                transform_matrix[3][2]));
  gtrsf.SetValues(m[0][0], m[1][0], m[2][0], m[3][0],
                  m[0][1], m[1][1], m[2][1], m[3][1],
                  m[0][2], m[1][2], m[2][2], m[3][2]
                  /*m[0][3], m[1][3], m[2][3], m[3][3]*/);
  return gtrsf;
}

void Transform(Handle(TDocStd_Document)& document,
                const glm::mat4x4& transform_matrix) {
  if (document.IsNull())
    return;

  gp_Trsf gtrsf = GlmToGTrsf(transform_matrix);
  auto shape_tool = XCAFDoc_DocumentTool::ShapeTool(document->Main());

  TDF_LabelSequence free_shapes;
  shape_tool->GetFreeShapes(free_shapes);
  for (const TDF_Label& label : free_shapes) {
    TopoDS_Shape shape;
    if (!shape_tool->GetShape(label, shape))
      continue;

    BRepBuilderAPI_Transform gtransform(shape, gtrsf);
    if (!gtransform.IsDone())
      continue;

    shape_tool->SetShape(label, gtransform.Shape());
  }
}

gp_Pnt TransformPoint(const gp_Pnt& point, const glm::mat4x4& m) {
  const double x = point.X();
  const double y = point.Y();
  const double z = point.Z();
  return gp_Pnt(m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3],
                m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3],
                m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3]);
}

void Transform(Handle(Poly_Triangulation)& trng,
                const glm::mat4x4& m) {
  if (trng.IsNull())
    return;

  const Standard_Integer nb_nodes = trng->NbNodes();
  for (Standard_Integer i = 1; i <= nb_nodes; ++i) {
    gp_Pnt orig = trng->Node(i);
    gp_Pnt next = TransformPoint(orig, m);
    trng->SetNode(i, TransformPoint(trng->Node(i), m));
  }
}
}

Outcome Workbench_OCC::LoadFromFile(const std::filesystem::path& filepath,
                                    const glm::mat4x4& transform_matrix) {
  Scene* scene = this->GetScene();

  Outcome outcome;
  Handle(Standard_Transient) loaded_object = ::LoadFromFile(filepath, &outcome);
  if (!outcome) {
    return Outcome::Error(
        std::format("Ошибка при загрузке файла '{}'",
                    filepath.string()));
  }

  if (loaded_object->IsKind(STANDARD_TYPE(TDocStd_Document))) {
    auto next_doc = Handle(TDocStd_Document)::DownCast(loaded_object);
    Transform(next_doc, transform_matrix);
    Append(document_, next_doc);
  } else if (loaded_object->IsKind(STANDARD_TYPE(Poly_Triangulation))) {
    auto trng = Handle(Poly_Triangulation)::DownCast(loaded_object);
    Transform(trng, transform_matrix);
    Append(document_, trng);
  } else {
    return Outcome::Error(std::format("Неизвестный тип объекта."));
  }

  this->Update();
  scene->FitScene();
  return Outcome::Success();
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

#include "command_changevisibility.h"
REGISTER_COMMAND(OCC, Command_ChangeVisibility, "change-visibility");

#include "command_loadfromfile.h"
REGISTER_COMMAND(OCC, Command_LoadFromFile, "load-from-file");
