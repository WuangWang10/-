#pragma once

#include <filesystem>

#include "lqr_sim/lqr_controller.hpp"

namespace lqr_sim {

struct SimulationConfig {
  double sample_time{0.01};
  double duration{30.0};
  double radius{5.0};
  double angular_rate{0.4};
  double max_acceleration{8.0};
  State initial_state{7.0, -1.0, 0.0, 0.0};
  StateMatrix q{20.0, 0.0, 0.0, 0.0,
                0.0, 20.0, 0.0, 0.0,
                0.0, 0.0, 5.0, 0.0,
                0.0, 0.0, 0.0, 5.0};
  InputWeight r{1.0, 0.0, 0.0, 1.0};
};

struct SimulationMetrics {
  double position_rmse{};
  double steady_state_rmse{};
  double maximum_position_error{};
  double final_position_error{};
  LqrResult lqr;
};

[[nodiscard]] SimulationMetrics RunSimulation(
    const SimulationConfig& config, const std::filesystem::path& csv_path);

}  // namespace lqr_sim
