#include "sceneiterators.h"

#include "numgeom/drawable.h"
#include "numgeom/trimeshconnectivity.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject.h"

namespace {
class SceneVertexIterator : public IteratorImpl<glm::vec3> {
 public:

  SceneVertexIterator(const Scene& scene) {
    it_sceneobject_ = scene.Objects();
    if(!it_sceneobject_.isEnd()) {
      it_drawable_ = (*it_sceneobject_)->Drawables();
      if(!it_drawable_.isEnd()) {
        it_vertex_ = (*it_drawable_)->GetVertices();
      }
    }
  }

  SceneVertexIterator(
      const Iterator<SceneObject*>& it_sceneobject,
      const Iterator<Drawable*>& it_drawable = Iterator<Drawable*>(),
      const Iterator<glm::vec3>& it_vertex = Iterator<glm::vec3>()) {
    it_sceneobject_ = it_sceneobject;
    it_drawable_ = it_drawable;
    it_vertex_ = it_vertex;
  }

  virtual ~SceneVertexIterator() {}

  void advance() override {
    ++it_vertex_;
    if(it_vertex_.isEnd()) {
      ++it_drawable_;
      if(it_drawable_.isEnd()) {
        ++it_sceneobject_;
        if(it_sceneobject_.isEnd())
          return;
        it_drawable_ = (*it_sceneobject_)->Drawables();
      }
      it_vertex_ = (*it_drawable_)->GetVertices();
    }
  }

  glm::vec3 current() const override { return (*it_vertex_); }

  IteratorImpl<glm::vec3>* clone() const override {
    return new SceneVertexIterator(it_sceneobject_, it_drawable_, it_vertex_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    return new SceneVertexIterator(it_sceneobject_.end());
  }

  bool end() const override { return it_sceneobject_.isEnd(); }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const SceneVertexIterator*>(&other);
    if(!ptr)
      return false;
    if(it_sceneobject_.isEnd() && it_sceneobject_ == ptr->it_sceneobject_)
      return true;
    return it_sceneobject_ == ptr->it_sceneobject_
        && it_drawable_ == ptr->it_drawable_
        && it_vertex_ == ptr->it_vertex_;
  }

 private:
  Iterator<SceneObject*> it_sceneobject_;
  Iterator<Drawable*> it_drawable_;
  Iterator<glm::vec3> it_vertex_;
};
}; // namespace

Iterator<glm::vec3> GetVertexIterator(const Scene& scene) {
  auto impl = new SceneVertexIterator(scene);
  return Iterator<glm::vec3>(impl);
}

namespace {
class SceneTriaIterator : public IteratorImpl<glm::u32vec3> {
 public:

   SceneTriaIterator(const Scene& scene) {
     it_sceneobject_ = scene.Objects();
     if(!it_sceneobject_.isEnd()) {
       it_drawable_ = GetTriaDrawables((*it_sceneobject_)->Drawables());
       if(!it_drawable_.isEnd()) {
         it_tria_ = (*it_drawable_)->GetTriangles();
       }
     }
   }

   SceneTriaIterator(
       const Iterator<SceneObject*>& it_sceneobject,
       const Iterator<Drawable2*>& it_drawable = Iterator<Drawable2*>(),
       const Iterator<glm::u32vec3>& it_tria = Iterator<glm::u32vec3>()) {
     it_sceneobject_ = it_sceneobject;
     it_drawable_ = it_drawable;
     it_tria_ = it_tria;
   }

  virtual ~SceneTriaIterator() {}

  void advance() override {
    ++it_tria_;
    if(it_tria_.isEnd()) {
      ++it_drawable_;
      if(it_drawable_.isEnd()) {
        ++it_sceneobject_;
        if(it_sceneobject_.isEnd())
          return;
        it_drawable_ = GetTriaDrawables((*it_sceneobject_)->Drawables());
      }
      it_tria_ = (*it_drawable_)->GetTriangles();
    }
  }

  glm::u32vec3 current() const override { return (*it_tria_); }

  IteratorImpl<glm::u32vec3>* clone() const override {
    return new SceneTriaIterator(it_sceneobject_, it_drawable_, it_tria_);
  }

  IteratorImpl<glm::u32vec3>* last() const override {
    return new SceneTriaIterator(it_sceneobject_.end());
  }

  bool end() const override { return it_sceneobject_.isEnd(); }

  bool equals(const IteratorImpl<glm::u32vec3>& other) const override {
    auto other_ptr = dynamic_cast<const SceneTriaIterator*>(&other);
    if(!other_ptr)
      return false;
    if(it_sceneobject_.isEnd() && it_sceneobject_ == other_ptr->it_sceneobject_)
      return true;
    return it_sceneobject_ == other_ptr->it_sceneobject_
        && it_drawable_ == other_ptr->it_drawable_
        && it_tria_ == other_ptr->it_tria_;
  }

 private:
  Iterator<SceneObject*> it_sceneobject_;
  Iterator<Drawable2*> it_drawable_;
  Iterator<glm::u32vec3> it_tria_;
};
}

Iterator<glm::u32vec3> GetTriaIterator(const Scene& scene) {
  auto impl = new SceneTriaIterator(scene);
  return Iterator<glm::u32vec3>(impl);
}

namespace {
class IteratorImpl_DrawableToDrawable2 : public IteratorImpl<Drawable2*> {
 public:

  IteratorImpl_DrawableToDrawable2(const Iterator<Drawable*>& it) : it_(it) {
    while(!it_.isEnd() && (*it_)->Type() != Drawable::PrimitiveType::Triangles)
      ++it_;
  }

  virtual ~IteratorImpl_DrawableToDrawable2() {}

  void advance() override {
    ++it_;
    while(!it_.isEnd() && (*it_)->Type() != Drawable::PrimitiveType::Triangles)
      ++it_;
  }

  Drawable2* current() const override {
    assert(!it_.isEnd());
    return Drawable2::Cast(*it_);
  }

  IteratorImpl<Drawable2*>* clone() const override {
    return new IteratorImpl_DrawableToDrawable2(it_);
  }

  IteratorImpl<Drawable2*>* last() const override {
    return new IteratorImpl_DrawableToDrawable2(it_.end());
  }

  bool end() const override {
    return it_.isEnd();
  }

  bool equals(const IteratorImpl<Drawable2*>& other) const override {
    auto other_ptr = dynamic_cast<const IteratorImpl_DrawableToDrawable2*>(&other);
    if(!other_ptr)
      return false;
    return it_ == other_ptr->it_;
  }

 private:
  Iterator<Drawable*> it_;
};
}

Iterator<Drawable2*> GetTriaDrawables(const Iterator<Drawable*>& it_drawable) {
  auto impl = new IteratorImpl_DrawableToDrawable2(it_drawable);
  return Iterator<Drawable2*>(impl);
}

namespace {
class IteratorImpl_Drawable2Normals : public IteratorImpl<glm::vec3> {
 public:

   IteratorImpl_Drawable2Normals(const Scene& scene) {
     it_sceneobject_ = scene.Objects();
     if(!it_sceneobject_.isEnd()) {
       it_drawable_ = GetTriaDrawables((*it_sceneobject_)->Drawables());
       if(!it_drawable_.isEnd()) {
         it_normal_ = (*it_drawable_)->GetNormals();
       }
     }
   }

   IteratorImpl_Drawable2Normals(
     const Iterator<SceneObject*>& it_sceneobject,
     const Iterator<Drawable2*>& it_drawable = Iterator<Drawable2*>(),
     const Iterator<glm::vec3>& it_normal = Iterator<glm::vec3>()) {
     it_sceneobject_ = it_sceneobject;
     it_drawable_ = it_drawable;
     it_normal_ = it_normal;
   }

  virtual ~IteratorImpl_Drawable2Normals() {}

  void advance() override {
    ++it_normal_;
    if(it_normal_.isEnd()) {
      ++it_drawable_;
      if(it_drawable_.isEnd()) {
        ++it_sceneobject_;
        if(it_sceneobject_.isEnd())
          return;
        it_drawable_ = GetTriaDrawables((*it_sceneobject_)->Drawables());
      }
      it_normal_ = (*it_drawable_)->GetNormals();
    }
  }

  glm::vec3 current() const override { return (*it_normal_); }

  IteratorImpl<glm::vec3>* clone() const override {
    return new IteratorImpl_Drawable2Normals(it_sceneobject_, it_drawable_,
                                             it_normal_);
  }

  IteratorImpl<glm::vec3>* last() const override {
    return new IteratorImpl_Drawable2Normals(it_sceneobject_.end());
  }

  bool end() const override { return it_sceneobject_.isEnd(); }

  bool equals(const IteratorImpl<glm::vec3>& other) const override {
    auto ptr = dynamic_cast<const IteratorImpl_Drawable2Normals*>(&other);
    if (!ptr)
      return false;
    if(it_sceneobject_.isEnd() && it_sceneobject_ == ptr->it_sceneobject_)
      return true;
    return it_sceneobject_ == ptr->it_sceneobject_
        && it_drawable_ == ptr->it_drawable_
        && it_normal_ == ptr->it_normal_;
  }

 private:
  Iterator<SceneObject*> it_sceneobject_;
  Iterator<Drawable2*> it_drawable_;
  Iterator<glm::vec3> it_normal_;
};
}

Iterator<glm::vec3> GetNormalIterator(const Scene& scene) {
  auto impl = new IteratorImpl_Drawable2Normals(scene);
  return Iterator<glm::vec3>(impl);
}
