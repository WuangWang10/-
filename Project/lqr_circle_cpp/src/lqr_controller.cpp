#include "lqr_sim/lqr_controller.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace lqr_sim {
namespace {

constexpr std::size_t kMaximumIterations = 10'000;
constexpr double kConvergenceTolerance = 1e-10;

}  // namespace

LqrController::LqrController(const StateMatrix& a, const InputMatrix& b,
                             const StateMatrix& q, const InputWeight& r,
                             double max_acceleration)
    : solution_(SolveDare(a, b, q, r)),
      max_acceleration_(max_acceleration) {
  if (max_acceleration_ <= 0.0) {
    throw std::invalid_argument("Maximum acceleration must be positive");
  }
}

Input LqrController::Calculate(const State& state,
                               const Reference& reference) const {
  const State error = state - reference.state;
  Input command = reference.acceleration - solution_.gain * error;

  // Limit the vector magnitude, matching a realistic acceleration constraint.
  const double magnitude = std::hypot(command(0, 0), command(1, 0));
  if (magnitude > max_acceleration_) {
    const double scale = max_acceleration_ / magnitude;
    command(0, 0) *= scale;
    command(1, 0) *= scale;
  }
  return command;
}

LqrResult LqrController::SolveDare(const StateMatrix& a, const InputMatrix& b,
                                   const StateMatrix& q,
                                   const InputWeight& r) {
  StateMatrix p = q;
  const StateMatrix at = Transpose(a);
  const Matrix<2, 4> bt = Transpose(b);
  double residual = 0.0;

  for (std::size_t iteration = 1; iteration <= kMaximumIterations;
       ++iteration) {
    const InputWeight s = r + bt * p * b;
    const GainMatrix gain = Inverse(s) * bt * p * a;
    const StateMatrix next_p = at * p * a - at * p * b * gain + q;
    residual = MaxAbsDifference(next_p, p);
    p = next_p;

    if (residual < kConvergenceTolerance) {
      const GainMatrix final_gain = Inverse(r + bt * p * b) * bt * p * a;
      return {final_gain, iteration, residual};
    }
  }
  throw std::runtime_error("DARE did not converge");
}

}  // namespace lqr_sim
