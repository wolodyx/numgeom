#ifndef NUMGEOM_FRAMEWORK_PHANTOM_H
#define NUMGEOM_FRAMEWORK_PHANTOM_H

#include "numgeom/drawable.h"
#include "numgeom/sceneobject.h"

class Drawable0_Cached;
class Drawable1_Cached;
class Drawable2_Cached;

class FRAMEWORK_EXPORT Phantom : public SceneObject {
 public:
  Phantom(Scene*);
  virtual ~Phantom();

  Drawable2_Cached* GetSurface() const;
  Drawable1_Cached* GetWireframe() const;
  Drawable0_Cached* GetPointCloud() const;

  void ClearData();
  void SetTransformMatrix(const glm::mat4x4&);

 private:
  Drawable0_Cached* drawable0_ = nullptr;
  Drawable1_Cached* drawable1_ = nullptr;
  Drawable2_Cached* drawable2_ = nullptr;
};
#endif // !NUMGEOM_FRAMEWORK_PHANTOM_H
