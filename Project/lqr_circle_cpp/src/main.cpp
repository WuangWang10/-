#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "lqr_sim/simulation.hpp"

namespace {

struct CommandLineOptions {
  lqr_sim::SimulationConfig simulation;
  std::filesystem::path output{"results/trajectory.csv"};
  bool show_help{false};
};

double ParseDouble(std::string_view option, const char* value) {
  try {
    std::size_t parsed = 0;
    const std::string text(value);
    const double result = std::stod(text, &parsed);
    if (parsed != text.size()) {
      throw std::invalid_argument("trailing characters");
    }
    return result;
  } catch (const std::exception&) {
    throw std::invalid_argument("Invalid value for " + std::string(option) +
                                ": " + value);
  }
}

CommandLineOptions ParseArguments(int argc, char* argv[]) {
  CommandLineOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "--help" || argument == "-h") {
      options.show_help = true;
      continue;
    }
    if (index + 1 >= argc) {
      throw std::invalid_argument("Missing value for " + std::string(argument));
    }
    const char* value = argv[++index];
    if (argument == "--dt") {
      options.simulation.sample_time = ParseDouble(argument, value);
    } else if (argument == "--duration") {
      options.simulation.duration = ParseDouble(argument, value);
    } else if (argument == "--radius") {
      options.simulation.radius = ParseDouble(argument, value);
    } else if (argument == "--omega") {
      options.simulation.angular_rate = ParseDouble(argument, value);
    } else if (argument == "--max-accel") {
      options.simulation.max_acceleration = ParseDouble(argument, value);
    } else if (argument == "--output") {
      options.output = value;
    } else {
      throw std::invalid_argument("Unknown option: " + std::string(argument));
    }
  }
  return options;
}

void PrintHelp() {
  std::cout
      << "2D point-mass circular trajectory tracking with discrete LQR\n\n"
      << "Usage: lqr_circle [options]\n"
      << "  --dt <seconds>       Sample time (default: 0.01)\n"
      << "  --duration <seconds> Simulation duration (default: 30)\n"
      << "  --radius <meters>    Circle radius (default: 5)\n"
      << "  --omega <rad/s>      Angular rate (default: 0.4)\n"
      << "  --max-accel <m/s^2>  Acceleration magnitude limit (default: 8)\n"
      << "  --output <path>      CSV output path\n"
      << "  -h, --help           Show this help\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  try {
    const CommandLineOptions options = ParseArguments(argc, argv);
    if (options.show_help) {
      PrintHelp();
      return EXIT_SUCCESS;
    }

    const lqr_sim::SimulationMetrics metrics =
        lqr_sim::RunSimulation(options.simulation, options.output);
    std::cout << std::fixed << std::setprecision(6)
              << "LQR DARE iterations : " << metrics.lqr.iterations << '\n'
              << "DARE residual       : " << metrics.lqr.residual << '\n'
              << "LQR gain K          : ["
              << metrics.lqr.gain(0, 0) << ", " << metrics.lqr.gain(0, 1)
              << ", " << metrics.lqr.gain(0, 2) << ", "
              << metrics.lqr.gain(0, 3) << "]\n"
              << "                      ["
              << metrics.lqr.gain(1, 0) << ", " << metrics.lqr.gain(1, 1)
              << ", " << metrics.lqr.gain(1, 2) << ", "
              << metrics.lqr.gain(1, 3) << "]\n"
              << "Position RMSE       : " << metrics.position_rmse << " m\n"
              << "Steady-state RMSE   : " << metrics.steady_state_rmse << " m\n"
              << "Maximum error       : " << metrics.maximum_position_error
              << " m\n"
              << "Final error         : " << metrics.final_position_error << " m\n"
              << "CSV written to      : " << options.output.string() << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
