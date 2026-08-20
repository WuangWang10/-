#pragma once

#include <cstddef>

#include "lqr_sim/types.hpp"

namespace lqr_sim {

struct LqrResult {
  GainMatrix gain;
  std::size_t iterations{};
  double residual{};
};

class LqrController {
 public:
  LqrController(const StateMatrix& a, const InputMatrix& b,
                const StateMatrix& q, const InputWeight& r,
                double max_acceleration);

  [[nodiscard]] Input Calculate(const State& state,
                                const Reference& reference) const;
  [[nodiscard]] const LqrResult& solution() const noexcept { return solution_; }

 private:
  [[nodiscard]] static LqrResult SolveDare(const StateMatrix& a,
                                           const InputMatrix& b,
                                           const StateMatrix& q,
                                           const InputWeight& r);

  LqrResult solution_;
  double max_acceleration_;
};

}  // namespace lqr_sim
