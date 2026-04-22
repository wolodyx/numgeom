#ifndef NUMGEOM_OCC_SCENEOBJECT_TDOCSTD_DOCUMENT_H
#define NUMGEOM_OCC_SCENEOBJECT_TDOCSTD_DOCUMENT_H

#include "Standard_Handle.hxx"

#include "numgeom/sceneobject.h"

class TDocStd_Document;

class SceneObject_TDocStd_Document : public SceneObject {
 public:
  SceneObject_TDocStd_Document(Scene*, const Handle(TDocStd_Document)&);
  virtual ~SceneObject_TDocStd_Document();

 private:
  const Handle(TDocStd_Document) document_;
};
#endif // !NUMGEOM_OCC_SCENEOBJECT_TDOCSTD_DOCUMENT_H
