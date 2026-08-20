#include "lqr_sim/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <stdexcept>

#include "lqr_sim/circle_trajectory.hpp"
#include "lqr_sim/point_mass.hpp"

namespace lqr_sim {

SimulationMetrics RunSimulation(const SimulationConfig& config,
                                const std::filesystem::path& csv_path) {
  if (config.sample_time <= 0.0 || config.duration <= 0.0) {
    throw std::invalid_argument("Sample time and duration must be positive");
  }

  PointMass2D plant(config.sample_time, config.initial_state);
  CircleTrajectory trajectory(config.radius, config.angular_rate);
  LqrController controller(plant.state_matrix(), plant.input_matrix(), config.q,
                           config.r, config.max_acceleration);

  if (!csv_path.parent_path().empty()) {
    std::filesystem::create_directories(csv_path.parent_path());
  }
  std::ofstream csv(csv_path);
  if (!csv) {
    throw std::runtime_error("Could not open CSV output: " + csv_path.string());
  }
  csv << "time,ref_x,ref_y,ref_vx,ref_vy,x,y,vx,vy,ax_cmd,ay_cmd,position_error\n";
  csv << std::fixed << std::setprecision(8);

  const std::size_t steps =
      static_cast<std::size_t>(std::ceil(config.duration / config.sample_time));
  const double steady_start = config.duration * 0.5;
  double squared_error_sum = 0.0;
  double steady_squared_error_sum = 0.0;
  std::size_t steady_samples = 0;
  SimulationMetrics metrics;

  for (std::size_t step = 0; step <= steps; ++step) {
    const double time = static_cast<double>(step) * config.sample_time;
    const Reference reference = trajectory.Sample(time);
    const Input command = controller.Calculate(plant.state(), reference);
    const State& state = plant.state();
    const double position_error =
        std::hypot(state(0, 0) - reference.state(0, 0),
                   state(1, 0) - reference.state(1, 0));

    squared_error_sum += position_error * position_error;
    metrics.maximum_position_error =
        std::max(metrics.maximum_position_error, position_error);
    metrics.final_position_error = position_error;
    if (time >= steady_start) {
      steady_squared_error_sum += position_error * position_error;
      ++steady_samples;
    }

    csv << time << ',' << reference.state(0, 0) << ',' << reference.state(1, 0)
        << ',' << reference.state(2, 0) << ',' << reference.state(3, 0) << ','
        << state(0, 0) << ',' << state(1, 0) << ',' << state(2, 0) << ','
        << state(3, 0) << ',' << command(0, 0) << ',' << command(1, 0) << ','
        << position_error << '\n';

    if (step < steps) {
      plant.Step(command);
    }
  }

  const double sample_count = static_cast<double>(steps + 1);
  metrics.position_rmse = std::sqrt(squared_error_sum / sample_count);
  metrics.steady_state_rmse =
      std::sqrt(steady_squared_error_sum / static_cast<double>(steady_samples));
  metrics.lqr = controller.solution();
  return metrics;
}

}  // namespace lqr_sim
