#include "lqr_sim/circle_trajectory.hpp"

#include <cmath>
#include <stdexcept>

namespace lqr_sim {

CircleTrajectory::CircleTrajectory(double radius, double angular_rate,
                                   double center_x, double center_y)
    : radius_(radius),
      angular_rate_(angular_rate),
      center_x_(center_x),
      center_y_(center_y) {
  if (radius_ <= 0.0) {
    throw std::invalid_argument("Radius must be positive");
  }
}

Reference CircleTrajectory::Sample(double time) const {
  const double angle = angular_rate_ * time;
  const double cosine = std::cos(angle);
  const double sine = std::sin(angle);
  const double rate_squared = angular_rate_ * angular_rate_;

  Reference reference;
  reference.state(0, 0) = center_x_ + radius_ * cosine;
  reference.state(1, 0) = center_y_ + radius_ * sine;
  reference.state(2, 0) = -radius_ * angular_rate_ * sine;
  reference.state(3, 0) = radius_ * angular_rate_ * cosine;
  reference.acceleration(0, 0) = -radius_ * rate_squared * cosine;
  reference.acceleration(1, 0) = -radius_ * rate_squared * sine;
  return reference;
}

}  // namespace lqr_sim
