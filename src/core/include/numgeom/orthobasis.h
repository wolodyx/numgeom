#ifndef NUMGEOM_CORE_ORTHOBASIS_H
#define NUMGEOM_CORE_ORTHOBASIS_H

#include <cassert>
#include <cmath>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

/**
\class OrthoBasis
\brief Ортонормированный базис в трёхмерном пространстве.

Хранит три взаимно перпендикулярных единичных вектора, образующих правую тройку.
Обычно обозначаются как X (right), Y (up), Z (forward).
*/
template<typename T>
class OrthoBasis {
 public:
  using Vec3 = glm::tvec3<T>;
  using Mat3 = glm::tmat3x3<T>;
  using Mat4 = glm::tmat4x4<T>;

  //! Конструктор по умолчанию создаёт базис, совпадающий с мировыми осями.
  OrthoBasis()
      : x_(1, 0, 0)
      , y_(0, 1, 0)
      , z_(0, 0, 1)
  {}

  //! Конструктор от трёх векторов (должны быть ортонормированы).
  OrthoBasis(const Vec3& x, const Vec3& y, const Vec3& z)
      : x_(x), y_(y), z_(z)
  {
    assert(IsOrthonormal() && "Vectors must be orthonormal");
  }

  //! Создаёт базис из матрицы поворота 3x3 (столбцы матрицы — векторы базиса).
  static OrthoBasis FromRotationMatrix(const Mat3& rot) {
    return OrthoBasis(
        Vec3(rot[0][0], rot[1][0], rot[2][0]),  // первый столбец
        Vec3(rot[0][1], rot[1][1], rot[2][1]),  // второй столбец
        Vec3(rot[0][2], rot[1][2], rot[2][2])   // третий столбец
    );
  }

  //! Создаёт базис из матрицы преобразования 4x4 (берётся только ротационная часть).
  static OrthoBasis FromTransformMatrix(const Mat4& mat) {
    Mat3 rot(mat);
    return FromRotationMatrix(rot);
  }

  //! Создаёт базис, где ось Z совпадает с заданным направлением,
  //! а оси X и Y выбираются произвольно, но ортонормированно.
  static OrthoBasis FromDirection(const Vec3& dir) {
    Vec3 z = glm::normalize(dir);
    // Выбираем произвольный вектор, не коллинеарный с z
    Vec3 up(0, 1, 0);
    if (std::abs(glm::dot(z, up)) > T(0.9)) {
      up = Vec3(1, 0, 0);
    }
    Vec3 x = glm::normalize(glm::cross(up, z));
    Vec3 y = glm::cross(z, x);
    return OrthoBasis(x, y, z);
  }

  //! Базис из двух векторов: ось X и приблизительная ось Y.
  static OrthoBasis FromZY(const Vec3& zDir, const Vec3& yApprox) {
    Vec3 z = glm::normalize(zDir);
    Vec3 x = glm::normalize(glm::cross(yApprox, z));
    Vec3 y = glm::cross(z, x);
    return OrthoBasis(x, y, z);
  }

  const Vec3& X() const { return x_; }
  const Vec3& Y() const { return y_; }
  const Vec3& Z() const { return z_; }

  const Vec3& GetRight() const { return x_; }
  const Vec3& GetUp() const { return y_; }
  const Vec3& GetForward() const { return z_; }

  //! Возвращает матрицу поворота 3x3, переводящую из локальных координат в мировые.
  //! Столбцы матрицы — векторы базиса.
  Mat3 ToRotationMatrix() const {
    return Mat3(
        x_.x, y_.x, z_.x,
        x_.y, y_.y, z_.y,
        x_.z, y_.z, z_.z
    );
  }

  //! Возвращает матрицу преобразования 4x4 (ротация + перевод).
  //! translation добавляется как смещение.
  Mat4 ToTransformMatrix(const Vec3& translation = Vec3(0)) const {
    return Mat4(
        x_.x, y_.x, z_.x, translation.x,
        x_.y, y_.y, z_.y, translation.y,
        x_.z, y_.z, z_.z, translation.z,
        0,    0,    0,    1
    );
  }

  //! Преобразует вектор из локальных координат базиса в мировые.
  Vec3 ToWorld(const Vec3& local) const {
    return x_ * local.x + y_ * local.y + z_ * local.z;
  }

  //! Преобразует вектор из мировых координат в локальные координаты базиса.
  Vec3 ToLocal(const Vec3& world) const {
    return Vec3(glm::dot(world, x_), glm::dot(world, y_), glm::dot(world, z_));
  }

  //! Проверяет, является ли базис ортонормированным с заданной точностью.
  bool IsOrthonormal(T epsilon = T(1e-6)) const {
    bool ortho = std::abs(glm::dot(x_, y_)) < epsilon &&
                 std::abs(glm::dot(y_, z_)) < epsilon &&
                 std::abs(glm::dot(z_, x_)) < epsilon;
    bool norm = std::abs(glm::length(x_) - T(1)) < epsilon &&
                std::abs(glm::length(y_) - T(1)) < epsilon &&
                std::abs(glm::length(z_) - T(1)) < epsilon;
    bool rightHanded = glm::dot(glm::cross(x_, y_), z_) > T(0);
    return ortho && norm && rightHanded;
  }

  //! Возвращает обратный базис (транспонированную матрицу поворота).
  OrthoBasis Inverse() const {
    // Для ортонормированного базиса обратный = транспонированный
    return OrthoBasis(
        Vec3(x_.x, y_.x, z_.x),
        Vec3(x_.y, y_.y, z_.y),
        Vec3(x_.z, y_.z, z_.z)
    );
  }

  //! Поворачивает базис вокруг заданной оси (в мировых координатах) на угол в радианах.
  OrthoBasis Rotate(const Vec3& axisWorld, T angleRad) const {
    Mat3 rot = glm::rotate(Mat3(1), angleRad, axisWorld);
    return FromRotationMatrix(rot * ToRotationMatrix());
  }

 private:
  Vec3 x_, y_, z_;
};
#endif // !NUMGEOM_CORE_ORTHOBASIS_H
