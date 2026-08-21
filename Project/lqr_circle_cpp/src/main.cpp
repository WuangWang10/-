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
  try {   // 尝试可能失败的代码
    std::size_t parsed = 0;
    const std::string text(value);    // 
    const double result = std::stod(text, &parsed);
    if (parsed != text.size()) {
      throw std::invalid_argument("trailing characters");
    }
    return result;
  } 
  catch (const std::exception&) {
    throw std::invalid_argument("Invalid value for " + std::string(option) +
                                ": " + value);
  }
}

CommandLineOptions ParseArguments(int argc, char* argv[]) {
  CommandLineOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);   // 获取参数名称 argument
    if (argument == "--help" || argument == "-h") {
      options.show_help = true;
      continue;
    }   // 这个 if 检查参数是否是 --help 或 -h，如果是就将 show_help 改为 true 并跳出循环（只显示帮助）。
    if (index + 1 >= argc) {
      throw std::invalid_argument("Missing value for " + std::string(argument));
    }   // 这个 if 检查参数名后面是否还有参数，如果没有就抛出异常。
    const char* value = argv[++index];    // 获取参数名称对应的值 value
    if (argument == "--dt") {
      options.simulation.sample_time = ParseDouble(argument, value);
    } 
    else if (argument == "--duration") {
      options.simulation.duration = ParseDouble(argument, value);
    } 
    else if (argument == "--radius") {
      options.simulation.radius = ParseDouble(argument, value);
    } 
    else if (argument == "--omega") {
      options.simulation.angular_rate = ParseDouble(argument, value);
    } 
    else if (argument == "--max-accel") {
      options.simulation.max_acceleration = ParseDouble(argument, value);
    }     // 除 output 以外的所有参数都需要 double 数值来表示，因此需要调用 ParseDouble 函数。
    else if (argument == "--output") {
      options.output = value;
    } 
    else {
      throw std::invalid_argument("Unknown option: " + std::string(argument));
    }   // 如果都不匹配，则抛出异常
  }
  return options;   // 返回更改之后的参数设置
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
}   // 打印使用说明，直接打印一串文本说明。

}  // namespace

int main(int argc, char* argv[]) {
  // try 表示尝试可能出错的代码，在中间任何一步抛出异常时，就转到 catch 统一处理。
  try {
    // 解析命令行参数，得到更新的 options 参数设置
    const CommandLineOptions options = ParseArguments(argc, argv);

    // 需要打印帮助就只打印帮助随后退出。
    if (options.show_help) {
      PrintHelp();
      return EXIT_SUCCESS;    // #define EXIT_SUCCESS = 0 这里相当于 return 0 退出程序。
    }

    // 在这一步运行仿真，得到结果
    const lqr_sim::SimulationMetrics metrics =
        lqr_sim::RunSimulation(options.simulation, options.output);

    // 格式化打印结果
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
  } 
  
  catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
