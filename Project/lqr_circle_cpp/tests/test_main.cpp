#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "lqr_sim/circle_trajectory.hpp"
#include "lqr_sim/matrix.hpp"
#include "lqr_sim/simulation.hpp"

namespace {

void ExpectNear(double actual, double expected, double tolerance,
                const std::string& message) {
  if (std::abs(actual - expected) > tolerance) {
    throw std::runtime_error(message + ": actual=" + std::to_string(actual) +
                             ", expected=" + std::to_string(expected));
  }
}

void TestMatrixInverse() {
  const lqr_sim::Matrix<2, 2> matrix{4.0, 7.0, 2.0, 6.0};
  const auto product = matrix * lqr_sim::Inverse(matrix);
  ExpectNear(product(0, 0), 1.0, 1e-12, "inverse[0,0]");
  ExpectNear(product(0, 1), 0.0, 1e-12, "inverse[0,1]");
  ExpectNear(product(1, 0), 0.0, 1e-12, "inverse[1,0]");
  ExpectNear(product(1, 1), 1.0, 1e-12, "inverse[1,1]");
}

void TestCircleReference() {
  const lqr_sim::CircleTrajectory trajectory(5.0, 0.4);
  const lqr_sim::Reference reference = trajectory.Sample(0.0);
  ExpectNear(reference.state(0, 0), 5.0, 1e-12, "reference x");
  ExpectNear(reference.state(1, 0), 0.0, 1e-12, "reference y");
  ExpectNear(reference.state(2, 0), 0.0, 1e-12, "reference vx");
  ExpectNear(reference.state(3, 0), 2.0, 1e-12, "reference vy");
  ExpectNear(reference.acceleration(0, 0), -0.8, 1e-12,
             "reference ax");
  ExpectNear(reference.acceleration(1, 0), 0.0, 1e-12,
             "reference ay");
}

void TestClosedLoopTracking() {
  lqr_sim::SimulationConfig config;
  config.duration = 15.0;
  const std::filesystem::path output =
      std::filesystem::temp_directory_path() / "lqr_circle_test.csv";
  const lqr_sim::SimulationMetrics metrics =
      lqr_sim::RunSimulation(config, output);

  if (metrics.lqr.residual >= 1e-9) {
    throw std::runtime_error("DARE residual is too large");
  }
  if (metrics.steady_state_rmse >= 0.01) {
    throw std::runtime_error("Steady-state tracking RMSE is too large: " +
                             std::to_string(metrics.steady_state_rmse));
  }
  if (!std::filesystem::exists(output) ||
      std::filesystem::file_size(output) == 0) {
    throw std::runtime_error("Simulation CSV was not created");
  }
}

}  // namespace

int main() {
  try {
    TestMatrixInverse();
    TestCircleReference();
    TestClosedLoopTracking();
    std::cout << "All tests passed.\n";
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "Test failed: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
