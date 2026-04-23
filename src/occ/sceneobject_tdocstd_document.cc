#include "numgeom/sceneobject_tdocstd_document.h"

#include "Quantity_Color.hxx"
#include "TDocStd_Document.hxx"
#include "XCAFDoc_DocumentTool.hxx"
#include "XCAFDoc_ColorTool.hxx"
#include "XCAFDoc_ShapeTool.hxx"

#include "numgeom/drawable_occshape.h"

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
    assert(shape.ShapeType() == TopAbs_COMPOUND);
    assert(!shape_tool->IsComponent(label));
    assert(!shape_tool->IsCompound(label));
    TDF_LabelSequence subLabels;
    bool r = shape_tool->GetComponents(label, subLabels);
    assert(r);
    for(const TDF_Label& lbl : subLabels)
      AnalyzeLabelRecursively(shape_tool, lbl, parent_trsf, triangulableShapes);
  }
  else if(shape_tool->IsComponent(label)) {
    assert(!shape_tool->IsAssembly(label));
    assert(!shape_tool->IsCompound(label));
    TDF_Label refLabel;
    bool r = shape_tool->GetReferredShape(label, refLabel);
    assert(r);
    TopLoc_Location loc = shape_tool->GetLocation(label);
    gp_Trsf trsf = parent_trsf;
    trsf.Multiply(loc.Transformation());
    return AnalyzeLabelRecursively(shape_tool, refLabel, trsf, triangulableShapes);
  }
  else if(shape_tool->IsCompound(label)) {
    assert(!shape_tool->IsAssembly(label));
    assert(!shape_tool->IsComponent(label));
    TDF_LabelSequence subLabels;
    shape_tool->GetSubShapes(label, subLabels);
    for(const TDF_Label& lbl : subLabels)
      AnalyzeLabelRecursively(shape_tool, lbl, parent_trsf, triangulableShapes);
  }
  else {
    assert(shape_tool->IsSimpleShape(label));
    auto t = shape.ShapeType();
    if(t == TopAbs_VERTEX || t == TopAbs_EDGE || t == TopAbs_WIRE)
      return false;
    TerminalShape shapeInfo{
        .label = label,
        .shape = shape,
        .trsf = parent_trsf
    };
    if(t == TopAbs_SOLID || t == TopAbs_SHELL || t == TopAbs_FACE) {
      triangulableShapes.push_back(shapeInfo);
    }
    else if(t == TopAbs_COMPOUND) {
      TDF_LabelSequence subLabels;
      shape_tool->GetSubShapes(label, subLabels);
      if(subLabels.IsEmpty()) {
        triangulableShapes.push_back(shapeInfo);
      }
      else {
        for(const TDF_Label& lbl : subLabels) {
          TopoDS_Shape subShape;
          shape_tool->GetShape(lbl,subShape);
          AnalyzeLabelRecursively(shape_tool, lbl, parent_trsf, triangulableShapes);
        }
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

} // namespace

SceneObject_TDocStd_Document::SceneObject_TDocStd_Document(
    Scene* scene,
    const Handle(TDocStd_Document)& doc) : SceneObject(scene), document_(doc) {
  std::list<TerminalShape> triangulableShapes;
  TDF_LabelSequence free_shape_labels;
  auto shape_tool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
  shape_tool->GetFreeShapes(free_shape_labels);
  for (const TDF_Label& lbl : free_shape_labels) {
    assert(shape_tool->IsTopLevel(lbl) && shape_tool->IsShape(lbl));
    AnalyzeLabelRecursively(shape_tool, lbl, gp_Trsf(), triangulableShapes);
  }

  auto color_tool = XCAFDoc_DocumentTool::ColorTool(doc->Main());
  for (const auto& tShape : triangulableShapes) {
    auto d = this->AddDrawable<Drawable2_OccShape>(tShape.shape, tShape.trsf);
    Quantity_Color clr;
    if (!color_tool->GetColor(tShape.shape,XCAFDoc_ColorSurf,clr))
      clr = Quantity_Color(0.647,0.647,0.647,Quantity_TOC_RGB);
    d->SetColor(clr.Red(), clr.Green(), clr.Blue());
  }
}

SceneObject_TDocStd_Document::~SceneObject_TDocStd_Document() {
}
