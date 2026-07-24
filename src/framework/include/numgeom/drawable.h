#ifndef NUMGEOM_FRAMEWORK_DRAWABLE_H
#define NUMGEOM_FRAMEWORK_DRAWABLE_H

#include <cstddef>

#include "numgeom/alignedboundbox.h"
#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"
#include "numgeom/trackedobject.h"

class SceneObject;

class FRAMEWORK_EXPORT Drawable : public TrackedObject {
 public:
  enum class PrimitiveType {
    Triangles,
    Edges,
    Vertices
  };

 public:
  Drawable(SceneObject* parent);
  virtual ~Drawable();

  bool IsVisible() const;
  void SetVisibility(bool visible = true);

  bool IsPickable() const;
  void DisablePicking();
  void EnablePicking();

  bool IsSelected() const;
  void Select();
  void Deselect();

  bool IsHighlighted() const;
  void Highlight(bool on = true);

  uint32_t GetId() const;

  virtual PrimitiveType Type() const = 0;
  virtual size_t GetVertsCount() const = 0;
  virtual size_t GetCellsCount() const = 0;
  virtual Iterator<glm::vec3> GetVertices() const = 0;

  /**
  \brief Возвращает габаритную коробку.

  Если производный класс обладает более быстрым способом вычисления коробки, чем
  пройтись по всем вершинам и расширять коробку, то следует этой реализацией
  переопределить этот метод.
  */
  virtual AlignedBoundBox GetBoundBox() const;

  void SetColor(const glm::vec3& color);
  void SetColor(float r, float g, float b);
  void SetColor(int r, int g, int b);
  glm::vec3 GetColor() const;

 private:
  SceneObject* parent_;
  uint32_t id_;
  glm::vec3 color_;
  bool is_pickable_;
  bool is_selected_;
  bool is_visible_;
  bool is_highlighted_;
};

class FRAMEWORK_EXPORT Drawable0 : public Drawable {
 public:
  static Drawable0* Cast(Drawable*);
  static const Drawable0* Cast(const Drawable*);

 public:
  Drawable0(SceneObject*);
  virtual ~Drawable0();
  PrimitiveType Type() const override {
    return PrimitiveType::Vertices;
  }
};

class FRAMEWORK_EXPORT Drawable1 : public Drawable {
 public:
  static Drawable1* Cast(Drawable*);
  static const Drawable1* Cast(const Drawable*);

 public:
  Drawable1(SceneObject*);
  virtual ~Drawable1();
  PrimitiveType Type() const override {
    return PrimitiveType::Edges;
  }
};

class FRAMEWORK_EXPORT Drawable2 : public Drawable {
 public:
  static Drawable2* Cast(Drawable*);
  static const Drawable2* Cast(const Drawable*);

 public:
  Drawable2(SceneObject*);
  virtual ~Drawable2();
  PrimitiveType Type() const override {
    return PrimitiveType::Triangles;
  }
  virtual Iterator<glm::u32vec3> GetTriangles() const = 0;
  virtual Iterator<glm::vec3> GetNormals() const = 0;
};
#endif // !NUMGEOM_FRAMEWORK_DRAWABLE_H
