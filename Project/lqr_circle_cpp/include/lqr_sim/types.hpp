#pragma once

#include "lqr_sim/matrix.hpp"

namespace lqr_sim {

using State = Matrix<4, 1>;
using Input = Matrix<2, 1>;
using StateMatrix = Matrix<4, 4>;
using InputMatrix = Matrix<4, 2>;
using GainMatrix = Matrix<2, 4>;
using InputWeight = Matrix<2, 2>;

struct Reference {
  State state;
  Input acceleration;
};

}  // namespace lqr_sim
