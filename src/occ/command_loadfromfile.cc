#include "command_loadfromfile.h"

#include "numgeom/drawableimplementations.h"
#include "numgeom/gizmo_filepath.h"
#include "numgeom/gizmo_placein3d.h"
#include "numgeom/gizmo_realnumber.h"
#include "numgeom/phantom.h"

#include "loadusingocc.h"
#include "workbench_occ.h"

#include "BRep_Tool.hxx"
#include "BRepMesh_IncrementalMesh.hxx"
#include "gp_Pnt.hxx"
#include "gp_Trsf.hxx"
#include "Poly_Triangulation.hxx"
#include "TDataXtd_Triangulation.hxx"
#include "TDF_Attribute.hxx"
#include "TDF_ChildIterator.hxx"
#include "TDF_Label.hxx"
#include "TDF_LabelSequence.hxx"
#include "TDocStd_Document.hxx"
#include "TopExp_Explorer.hxx"
#include "TopoDS.hxx"
#include "TopoDS_Face.hxx"
#include "TopoDS_Shape.hxx"
#include "XCAFDoc_DocumentTool.hxx"
#include "XCAFDoc_ShapeTool.hxx"

Command_LoadFromFile::Command_LoadFromFile(Workbench* workbench)
    : Command_OCC(workbench) {
  gizmo_1_ = new Gizmo_FilePath(this, "Select file to load");
  gizmo_2_ = new Gizmo_PlaceIn3d(this, "Place model in scene", glm::vec3(1.0f, 2.0f, 3.0f));
  gizmo_3_ = new Gizmo_RealNumber(this, "Scale coeff", 1.0);

  gizmo_3_->SetNumber(1.0);
  gizmo_3_->SetFilter([](double x) { return x > 0.0; });

  this->Updated(gizmo_1_);
}

Command_LoadFromFile::~Command_LoadFromFile() {
}

Outcome Command_LoadFromFile::Execute() {
  auto filepath = GetModelFilePath();
  if (filepath.empty())
    return Outcome::Error("File path is empty.");
  if (!std::filesystem::exists(filepath))
    return Outcome::Error("File does not exist.");
  auto tr_matrix = GetModelTransformation();
  auto scale = GetScaleCoeff();
  if (scale <= 0.0)
    return Outcome::Error("Scale coeff is not positive.");
  tr_matrix = glm::scale(tr_matrix, glm::vec3(scale));
  auto workbench = this->GetUnderlyingWorkbench();
  workbench->LoadFromFile(filepath, tr_matrix);
  return Outcome::Success();
}

std::filesystem::path Command_LoadFromFile::GetModelFilePath() const {
  return gizmo_1_->GetFilePath();
}

glm::mat4x4 Command_LoadFromFile::GetModelTransformation() const {
  return gizmo_2_->GetTransformMatrix();
}

float Command_LoadFromFile::GetScaleCoeff() const {
  return gizmo_3_->GetNumber();
}

void Command_LoadFromFile::Recover() {
  //Command::Recover();
}

namespace {

struct TerminalShape {
  TDF_Label label;
  TopoDS_Shape shape;
  gp_Trsf trsf;
};

bool AnalyzeLabelRecursively(
  const Handle(XCAFDoc_ShapeTool)& shape_tool,
  const TDF_Label& label,
  const gp_Trsf& parent_trsf,
  std::list<TerminalShape>& triangulableShapes)
{
  TopoDS_Shape shape;
  assert(shape_tool->IsShape(label));
  shape_tool->GetShape(label, shape);
  assert(!shape.IsNull());

  if (shape_tool->IsAssembly(label)) {
    TDF_LabelSequence subLabels;
    bool r = shape_tool->GetComponents(label, subLabels);
    assert(r);
    for (const TDF_Label& lbl : subLabels)
      AnalyzeLabelRecursively(shape_tool, lbl, parent_trsf, triangulableShapes);
  }
  else if (shape_tool->IsComponent(label)) {
    TDF_Label refLabel;
    bool r = shape_tool->GetReferredShape(label, refLabel);
    assert(r);
    TopLoc_Location loc = shape_tool->GetLocation(label);
    gp_Trsf trsf = parent_trsf;
    trsf.Multiply(loc.Transformation());
    return AnalyzeLabelRecursively(shape_tool, refLabel, trsf, triangulableShapes);
  }
  else if (shape_tool->IsCompound(label)) {
    TDF_LabelSequence subLabels;
    shape_tool->GetSubShapes(label, subLabels);
    for (const TDF_Label& lbl : subLabels)
      AnalyzeLabelRecursively(shape_tool, lbl, parent_trsf, triangulableShapes);
  }
  else {
    assert(shape_tool->IsSimpleShape(label));
    auto t = shape.ShapeType();
    if (t == TopAbs_VERTEX || t == TopAbs_EDGE || t == TopAbs_WIRE)
      return false;
    if (t == TopAbs_SOLID || t == TopAbs_SHELL || t == TopAbs_FACE) {
      triangulableShapes.push_back({label, shape, parent_trsf});
    }
    else if (t == TopAbs_COMPOUND) {
      TDF_LabelSequence subLabels;
      shape_tool->GetSubShapes(label, subLabels);
      if (subLabels.IsEmpty()) {
        triangulableShapes.push_back({label, shape, parent_trsf});
      }
      else {
        for (const TDF_Label& lbl : subLabels)
          AnalyzeLabelRecursively(shape_tool, lbl, parent_trsf, triangulableShapes);
      }
    }
    else {
      // t == TopAbs_COMPSOLID
      assert(false);
      return false;
    }
  }

  return true;
}

void TriangulationToPhantom(const Handle(Poly_Triangulation)& trng,
                            Phantom* phantom);

void DocumentToPhantom(const Handle(TDocStd_Document)& document,
                       Phantom* phantom) {
  if (document.IsNull())
    return;
  auto wireframe = phantom->GetWireframe();
  auto surface = phantom->GetSurface();

  std::list<TerminalShape> triangulableShapes;
  TDF_LabelSequence free_shape_labels;
  auto shape_tool = XCAFDoc_DocumentTool::ShapeTool(document->Main());
  shape_tool->GetFreeShapes(free_shape_labels);
  for (const TDF_Label& lbl : free_shape_labels) {
    if (shape_tool->IsTopLevel(lbl) && shape_tool->IsShape(lbl))
      AnalyzeLabelRecursively(shape_tool, lbl, gp_Trsf(), triangulableShapes);
  }

  for (const auto& tShape : triangulableShapes) {
    for (TopExp_Explorer it(tShape.shape, TopAbs_FACE); it.More(); it.Next()) {
      const TopoDS_Face& f = TopoDS::Face(it.Value());
      TopLoc_Location l;
      Handle(Poly_Triangulation) tr = BRep_Tool::Triangulation(f, l);
      if (!tr) {
        BRepMesh_IncrementalMesh(f, 0.6f);
        tr = BRep_Tool::Triangulation(f, l);
      }
      if (!tr)
        continue;

      gp_Trsf trsf = tShape.trsf;
      trsf.Multiply(l.Transformation());

      uint32_t base = static_cast<uint32_t>(wireframe->GetVertsCount());
      const Standard_Integer nb_nodes = tr->NbNodes();
      for (Standard_Integer i = 1; i <= nb_nodes; ++i) {
        gp_Pnt pnt = tr->Node(i);
        pnt.Transform(trsf);
        glm::vec3 v(pnt.X(), pnt.Y(), pnt.Z());
        wireframe->AddPoint(v);
        surface->AddPoint(v);
      }

      const Standard_Integer nb_triangles = tr->NbTriangles();
      for (Standard_Integer i = 1; i <= nb_triangles; ++i) {
        const Poly_Triangle& tria = tr->Triangle(i);
        Standard_Integer i1, i2, i3;
        tria.Get(i1, i2, i3);
        --i1, --i2, --i3;
        uint32_t a = base + static_cast<uint32_t>(i1);
        uint32_t b = base + static_cast<uint32_t>(i2);
        uint32_t c = base + static_cast<uint32_t>(i3);
        wireframe->AddCell(a, b);
        wireframe->AddCell(b, c);
        wireframe->AddCell(c, a);
        surface->AddCell(a, b, c);
      }
    }
  }

  // Collect standalone Poly_Triangulation stored as TDataXtd_Triangulation
  // attributes (they are not XCAF free shapes).
  for (TDF_ChildIterator child_it(document->Main(), Standard_True);
       child_it.More(); child_it.Next()) {
    const TDF_Label& label = child_it.Value();
    Handle(TDF_Attribute) attr;
    if (!label.FindAttribute(TDataXtd_Triangulation::GetID(), attr))
      continue;
    auto trng_attr = Handle(TDataXtd_Triangulation)::DownCast(attr);
    if (trng_attr.IsNull())
      continue;
    Handle(Poly_Triangulation) trng = trng_attr->Get();
    if (trng.IsNull())
      continue;
    TriangulationToPhantom(trng, phantom);
  }
}

// Convert a single triangulation into points/edges of the phantom wireframe.
void TriangulationToPhantom(const Handle(Poly_Triangulation)& trng,
                            Phantom* phantom) {
  auto wireframe = phantom->GetWireframe();
  const Standard_Integer nb_nodes = trng->NbNodes();
  const Standard_Integer nb_triangles = trng->NbTriangles();
  if (nb_nodes == 0 || nb_triangles == 0)
    return;

  for (Standard_Integer i = 1; i <= nb_nodes; ++i) {
    const gp_Pnt pnt = trng->Node(i);
    wireframe->AddPoint(glm::vec3(pnt.X(), pnt.Y(), pnt.Z()));
  }

  for (Standard_Integer i = 1; i <= nb_triangles; ++i) {
    const Poly_Triangle& tria = trng->Triangle(i);
    Standard_Integer i1, i2, i3;
    tria.Get(i1, i2, i3);
    --i1, --i2, --i3;
    wireframe->AddCell(i1, i2);
    wireframe->AddCell(i2, i3);
    wireframe->AddCell(i3, i1);
  }
}

void FileToPhantom(const std::filesystem::path& filepath, Phantom* phantom) {
  auto loaded_object = LoadFromFile(filepath);
  if (loaded_object->IsKind(STANDARD_TYPE(TDocStd_Document))) {
    auto document = Handle(TDocStd_Document)::DownCast(loaded_object);
    DocumentToPhantom(document, phantom);
  } else if (loaded_object->IsKind(STANDARD_TYPE(Poly_Triangulation))) {
    auto trng = Handle(Poly_Triangulation)::DownCast(loaded_object);
    TriangulationToPhantom(trng, phantom);
  } else {
    return;
  }
}

float CalcCharacteristicSize(const Phantom* phantom) {
  auto box = phantom->GetBoundBox();
  auto box_size = box.GetSize();
  return std::min(box_size.x, std::min(box_size.y, box_size.z));
}
}

#include <iostream>
void Command_LoadFromFile::Updated(Gizmo* gizmo) {
  if (gizmo == gizmo_1_) {
    std::cout << "Command_LoadFromFile::Updated: gizmo_1_" << std::endl;
    auto filepath = gizmo_1_->GetFilePath();
    bool enable_next_gizmos = std::filesystem::exists(filepath);
    auto phantom = this->GetPhantom();
    if (!enable_next_gizmos) {
      gizmo_2_->SetEnabled(false);
      gizmo_3_->SetEnabled(false);
      phantom->ClearData();
      return;
    }
    gizmo_2_->SetEnabled(true);
    gizmo_3_->SetEnabled(true);
    FileToPhantom(filepath, this->GetPhantom());
    float ch_size = CalcCharacteristicSize(phantom);
    gizmo_2_->SetCharacteristicSize(ch_size);
  } else if (gizmo == gizmo_2_) {
    std::cout << "Command_LoadFromFile::Updated: gizmo_2_" << std::endl;
    auto m = gizmo_2_->GetTransformMatrix();
    this->GetPhantom()->SetTransformMatrix(m);
  } else if (gizmo == gizmo_3_) {
    std::cout << "Command_LoadFromFile::Updated: gizmo_3_" << std::endl;
  } else {
    std::abort();
  }
  this->UpdateScreen();
}
