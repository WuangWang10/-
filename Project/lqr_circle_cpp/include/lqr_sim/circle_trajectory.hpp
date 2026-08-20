#pragma once

#include "lqr_sim/types.hpp"

namespace lqr_sim {

class CircleTrajectory {
 public:
  CircleTrajectory(double radius, double angular_rate, double center_x = 0.0,
                   double center_y = 0.0);

  [[nodiscard]] Reference Sample(double time) const;

 private:
  double radius_;
  double angular_rate_;
  double center_x_;
  double center_y_;
};

}  // namespace lqr_sim
