#pragma once

#include "lqr_sim/types.hpp"

namespace lqr_sim {

class PointMass2D {
 public:
  PointMass2D(double sample_time, State initial_state);

  void Step(const Input& acceleration);
  [[nodiscard]] const State& state() const noexcept { return state_; }
  [[nodiscard]] const StateMatrix& state_matrix() const noexcept { return a_; }
  [[nodiscard]] const InputMatrix& input_matrix() const noexcept { return b_; }

 private:
  double sample_time_;
  State state_;
  StateMatrix a_;
  InputMatrix b_;
};

}  // namespace lqr_sim
