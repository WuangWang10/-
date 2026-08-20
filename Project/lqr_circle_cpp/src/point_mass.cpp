#include "lqr_sim/point_mass.hpp"

#include <stdexcept>

namespace lqr_sim {

PointMass2D::PointMass2D(double sample_time, State initial_state)
    : sample_time_(sample_time), state_(initial_state) {
  if (sample_time_ <= 0.0) {
    throw std::invalid_argument("Sample time must be positive");
  }

  a_ = StateMatrix::Identity();
  a_(0, 2) = sample_time_;
  a_(1, 3) = sample_time_;

  const double half_dt_squared = 0.5 * sample_time_ * sample_time_;
  b_(0, 0) = half_dt_squared;
  b_(1, 1) = half_dt_squared;
  b_(2, 0) = sample_time_;
  b_(3, 1) = sample_time_;
}

void PointMass2D::Step(const Input& acceleration) {
  state_ = a_ * state_ + b_ * acceleration;
}

}  // namespace lqr_sim
