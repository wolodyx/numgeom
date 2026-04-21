#ifndef NUMGEOM_CORE_ALIGNEDBOUNDBOX_H
#define NUMGEOM_CORE_ALIGNEDBOUNDBOX_H

#include <limits>
#include <cmath>
#include <algorithm>

#include <glm/glm.hpp>

#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif

/**
\class AlignedBoundBox
\brief Габаритная коробка, выровненная по осям координат.
*/
class AlignedBoundBox {
 public:
  glm::vec3 min_;
  glm::vec3 max_;

 public:
  //! Конструктор по умолчанию создает пустую коробку.
  AlignedBoundBox() 
      : min_(+std::numeric_limits<float>::max())
      , max_(-std::numeric_limits<float>::max())
  {}

  //! Конструктор от двух точек (предполагается, что min <= max)
  AlignedBoundBox(const glm::vec3& min, const glm::vec3& max)
    : min_(min), max_(max)
  {}

  bool IsEmpty() const {
    return min_.x > max_.x || min_.y > max_.y || min_.z > max_.z;
  }

  //! Расширение коробки, чтобы включить точку
  void Expand(const glm::vec3& point) {
    if (IsEmpty()) {
      min_ = max_ = point;
    } else {
      min_ = glm::min(min_, point);
      max_ = glm::max(max_, point);
    }
  }

  //! Расширение коробки другой коробкой
  void Expand(const AlignedBoundBox& other) {
    if (other.IsEmpty()) return;
    if (IsEmpty()) {
      *this = other;
    } else {
      min_ = glm::min(min_, other.min_);
      max_ = glm::max(max_, other.max_);
    }
  }

  //! Проверка пересечения с другой коробкой
  bool Intersects(const AlignedBoundBox& other) const {
    if (IsEmpty() || other.IsEmpty()) return false;
    return (min_.x <= other.max_.x && max_.x >= other.min_.x) &&
           (min_.y <= other.max_.y && max_.y >= other.min_.y) &&
           (min_.z <= other.max_.z && max_.z >= other.min_.z);
  }

  //! Объединение коробок (возвращает новую)
  AlignedBoundBox United(const AlignedBoundBox& other) const {
    AlignedBoundBox result = *this;
    result.Expand(other);
    return result;
  }

  //! Возвращает коробку, как результат пересечения двух коробок.
  AlignedBoundBox Intersection(const AlignedBoundBox& other) const {
    if (!Intersects(other)) return AlignedBoundBox(); // пустая
    return AlignedBoundBox(glm::max(min_, other.min_), glm::min(max_, other.max_));
  }

  //! Сравнение двух габаритных коробок с заданной точностью.
  bool IsEqual(const AlignedBoundBox& other, float tolerance) const {
    if (IsEmpty() && other.IsEmpty()) return true;
    if (IsEmpty() || other.IsEmpty()) return false;
    return std::abs(min_.x - other.min_.x) <= tolerance &&
           std::abs(min_.y - other.min_.y) <= tolerance &&
           std::abs(min_.z - other.min_.z) <= tolerance &&
           std::abs(max_.x - other.max_.x) <= tolerance &&
           std::abs(max_.y - other.max_.y) <= tolerance &&
           std::abs(max_.z - other.max_.z) <= tolerance;
  }

  //! Возвращает максимальное отклонение между двумя коробками.
  //! Вычисляется как наибольшая из абсолютных разностей соответствующих
  //! координат min и max. Если одна из коробок пуста, возвращает
  //! std::numeric_limits<float>::max().
  float Deviation(const AlignedBoundBox& other) const {
    if (IsEmpty() || other.IsEmpty()) {
      return std::numeric_limits<float>::max();
    }
    float dx_min = std::abs(min_.x - other.min_.x);
    float dy_min = std::abs(min_.y - other.min_.y);
    float dz_min = std::abs(min_.z - other.min_.z);
    float dx_max = std::abs(max_.x - other.max_.x);
    float dy_max = std::abs(max_.y - other.max_.y);
    float dz_max = std::abs(max_.z - other.max_.z);
    return std::max({dx_min, dy_min, dz_min, dx_max, dy_max, dz_max});
  }

  //! Оператор точного равенства (с точностью 1e-6).
  bool operator==(const AlignedBoundBox& other) const {
    return IsEqual(other, 1e-6f);
  }

  //! Оператор неравенства.
  bool operator!=(const AlignedBoundBox& other) const {
    return !(*this == other);
  }

  glm::vec3 GetCenter() const {
    if (IsEmpty()) return glm::vec3();
    return (min_ + max_) * 0.5f;
  }

  //! Размах коробки по всем трем осям.
  glm::vec3 GetSize() const {
    if (IsEmpty()) return glm::vec3(0.0f);
    return max_ - min_;
  }

  const glm::vec3& min() const { return min_; }
  const glm::vec3& max() const { return max_; }

  //! Сброс в пустое состояние.
  void Clear() {
    min_ = glm::vec3(+std::numeric_limits<float>::max());
    max_ = glm::vec3(-std::numeric_limits<float>::max());
  }

  //! Проверка точки на попадание внутрь коробки, включая границы.
  bool Contains(const glm::vec3& point) const {
    if (IsEmpty()) return false;
    return point.x >= min_.x && point.x <= max_.x &&
           point.y >= min_.y && point.y <= max_.y &&
           point.z >= min_.z && point.z <= max_.z;
  }
};
#endif // !NUMGEOM_CORE_ALIGNEDBOUNDBOX_H
