/* ----------------------------------------------------------------------------

 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/*
 * @file Sphere2.h
 * @date Feb 02, 2011
 * @author Can Erdogan
 * @author Frank Dellaert
 * @author Alex Trevor
 * @brief The Sphere2 class - basically a point on a unit sphere
 */

#include <gtsam/geometry/Sphere2.h>
#include <gtsam/geometry/Point2.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_on_sphere.hpp>
#include <iostream>

using namespace std;

namespace gtsam {

/* ************************************************************************* */
Sphere2 Sphere2::FromPoint3(const Point3& point, boost::optional<Matrix&> H) {
  Sphere2 direction(point);
  if (H) {
    // 3*3 Derivative of representation with respect to point is 3*3:
    Matrix D_p_point;
    point.normalize(D_p_point); // TODO, this calculates norm a second time :-(
    // Calculate the 2*3 Jacobian
    H->resize(2, 3);
    *H << direction.basis().transpose() * D_p_point;
  }
  return direction;
}

/* ************************************************************************* */
Sphere2 Sphere2::Random(boost::random::mt19937 & rng) {
  // TODO allow any engine without including all of boost :-(
  boost::random::uniform_on_sphere<double> randomDirection(3);
  vector<double> d = randomDirection(rng);
  Sphere2 result;
  result.p_ = Point3(d[0], d[1], d[2]);
  return result;
}

/* ************************************************************************* */
Matrix Sphere2::basis() const {

  // Return cached version if exists
  if (B_.rows() == 3)
    return B_;

  // Get the axis of rotation with the minimum projected length of the point
  Point3 axis;
  double mx = fabs(p_.x()), my = fabs(p_.y()), mz = fabs(p_.z());
  if ((mx <= my) && (mx <= mz))
    axis = Point3(1.0, 0.0, 0.0);
  else if ((my <= mx) && (my <= mz))
    axis = Point3(0.0, 1.0, 0.0);
  else if ((mz <= mx) && (mz <= my))
    axis = Point3(0.0, 0.0, 1.0);
  else
    assert(false);

  // Create the two basis vectors
  Point3 b1 = p_.cross(axis);
  b1 = b1 / b1.norm();
  Point3 b2 = p_.cross(b1);
  b2 = b2 / b2.norm();

  // Create the basis matrix
  B_ = Matrix(3, 2);
  B_ << b1.x(), b2.x(), b1.y(), b2.y(), b1.z(), b2.z();
  return B_;
}

/* ************************************************************************* */
/// The print fuction
void Sphere2::print(const std::string& s) const {
  cout << s << ":" << p_ << endl;
}

/* ************************************************************************* */
Matrix Sphere2::skew() const {
  return skewSymmetric(p_.x(), p_.y(), p_.z());
}

/* ************************************************************************* */
Vector Sphere2::error(const Sphere2& q, boost::optional<Matrix&> H) const {
  Matrix Bt = basis().transpose();
  Vector xi = Bt * q.p_.vector();
  if (H)
    *H = Bt * q.basis();
  return xi;
}

/* ************************************************************************* */
double Sphere2::distance(const Sphere2& q, boost::optional<Matrix&> H) const {
  Vector xi = error(q, H);
  double theta = xi.norm();
  if (H)
    *H = (xi.transpose() / theta) * (*H);
  return theta;
}

/* ************************************************************************* */
Sphere2 Sphere2::retract(const Vector& v) const {

  // Get the vector form of the point and the basis matrix
  Vector p = Point3::Logmap(p_);
  Matrix B = basis();

  // Compute the 3D xi_hat vector
  Vector xi_hat = v(0) * B.col(0) + v(1) * B.col(1);

  double xi_hat_norm = xi_hat.norm();

  // Avoid nan
  if (xi_hat_norm == 0.0) {
    if (v.norm() == 0.0)
      return Sphere2(point3());
    else
      return Sphere2(-point3());
  }

  Vector exp_p_xi_hat = cos(xi_hat_norm) * p
      + sin(xi_hat_norm) * (xi_hat / xi_hat_norm);
  return Sphere2(exp_p_xi_hat);

}

/* ************************************************************************* */
Vector Sphere2::localCoordinates(const Sphere2& y) const {

  Vector p = Point3::Logmap(p_);
  Vector q = Point3::Logmap(y.p_);
  double dot = p.dot(q);

  // Check for special cases
  if (std::abs(dot - 1.0) < 1e-20)
    return (Vector(2) << 0, 0);
  else if (std::abs(dot + 1.0) < 1e-20)
    return (Vector(2) << M_PI, 0);
  else {
    // no special case
    double theta = acos(dot);
    Vector result_hat = (theta / sin(theta)) * (q - p * dot);
    return basis().transpose() * result_hat;
  }
}
/* ************************************************************************* */

}
