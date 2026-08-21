# main.cpp 学习笔记

> **学习对象**：`src/main.cpp` —— LQR 圆轨迹仿真程序的命令行入口
> **读者定位**：C++ 初学者
> **笔记内容**：程序功能、逐段代码讲解、源码 → exe 的构建全过程、亲手实验记录、知识点速查

---

## 目录

1. [这个程序是干什么的](#一这个程序是干什么的)
2. [代码结构总览](#二代码结构总览)
3. [逐段代码详解](#三逐段代码详解)
4. [从 main.cpp 到 lqr_circle.exe](#四从-maincpp-到-lqrcircleexe)
5. [亲手实验记录](#五亲手实验记录)
6. [运行方法](#六运行方法)
7. [知识点速查表](#七知识点速查表)
8. [常见疑问 FAQ](#八常见疑问-faq)
9. [下一步学习建议](#九下一步学习建议)
10. [实战练习题](#十实战练习题面向实际工作技能)

---

## 一、这个程序是干什么的

一句话总结：

> **这是一个命令行工具：解析用户输入的参数 → 运行一个"LQR 控制器跟踪圆形轨迹"的仿真 → 把轨迹数据写成 CSV 文件 → 在屏幕上打印误差统计结果。**

这个文件是程序的**入口**（`main` 函数所在），它本身不做数学计算，而是负责"接活、派活、汇报结果"。真正的控制算法在 `lqr_sim` 库的其他文件里（`simulation.cpp`、`lqr_controller.cpp`、`point_mass.cpp`、`circle_trajectory.cpp`）。

程序运行示例输出：

```
LQR DARE iterations : 770
DARE residual       : 0.000000
LQR gain K          : [4.389413, 0.000000, 3.687216, 0.000000]
                      [0.000000, 4.389413, 0.000000, 3.687216]
Position RMSE       : 1.143262 m
Steady-state RMSE   : 0.160420 m
Maximum error       : 2.307662 m
Final error         : 0.004529 m
CSV written to      : results/demo.csv
```

---

## 二、代码结构总览

`main.cpp` 共 112 行，分三大块：

| 行号 | 内容 | 作用 |
|---|---|---|
| 1–9 | `#include` 头文件 | 引入标准库和项目头文件 |
| 11–78 | 匿名命名空间 `namespace { ... }` | 文件内部的"私有工具"：参数解析相关 |
| 80–112 | `main` 函数 | 程序入口：解析 → 仿真 → 打印 |

依赖的外部接口（来自 `include/lqr_sim/simulation.hpp`）：

- `lqr_sim::SimulationConfig` —— 仿真配置结构体（dt、时长、半径等，带默认值）
- `lqr_sim::SimulationMetrics` —— 结果统计结构体（各种误差 + LQR 求解信息）
- `lqr_sim::RunSimulation(config, csv_path)` —— 真正跑仿真的函数

---

## 三、逐段代码详解

### 3.1 头文件（第 1–9 行）

```cpp
#include <cstdlib>      // EXIT_SUCCESS / EXIT_FAILURE（程序退出码）
#include <filesystem>   // std::filesystem::path（路径类型，C++17 新增）
#include <iomanip>      // std::fixed、std::setprecision（控制输出格式）
#include <iostream>     // std::cout / std::cerr（屏幕输出）
#include <stdexcept>    // std::invalid_argument（抛出的异常类型）
#include <string>       // std::string（字符串）
#include <string_view>  // std::string_view（"只读的字符串视角"）

#include "lqr_sim/simulation.hpp"  // 项目自己的头文件，声明了仿真接口
```

**要点**：

- `#include <...>` 引入**标准库**（尖括号）；`#include "..."` 引入**项目自己的头文件**（引号）。
- `#include` 的本质是"把那个文件的全部内容复制粘贴到这一行"，仅此而已（后面实验会亲眼看到）。

### 3.2 匿名命名空间（第 11–78 行）

```cpp
namespace {
  ... 所有辅助函数和结构体 ...
}  // namespace
```

- 没有名字的命名空间 = **只在本文件内可见**，别的文件 include 也看不到。
- 用途：封装"内部实现细节"，不污染全局。看到它就理解为"这是本文件的私有工具"。

### 3.3 配置结构体 `CommandLineOptions`（第 13–17 行）

```cpp
struct CommandLineOptions {
  lqr_sim::SimulationConfig simulation;   // 仿真参数（默认值在 simulation.hpp 里）
  std::filesystem::path output{"results/trajectory.csv"};  // 输出路径，默认值
  bool show_help{false};                  // 是否只打印帮助
};
```

**要点**：

- `struct`（结构体）＝ 把几个变量打包成一个新类型。
- `成员{默认值}` 叫**默认成员初始化**：不设置就用默认值。
- `simulation` 成员的默认值（`dt=0.01`、`radius=5.0` 等）定义在 `simulation.hpp` 里，main.cpp 不用重复写。

### 3.4 `ParseDouble`（第 19–32 行）：字符串 → 数字

```cpp
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
```

**要点**：

- `std::stod(text, &parsed)`：字符串 → double，并把**已解析的字符数**写入 `parsed`。
- `parsed != text.size()` 检查：解析不完整（如 `"0.5abc"`）就主动抛异常。
- `try/catch`：先"吞掉"原始错误，再抛一个**信息更友好**的异常（告诉用户是哪个参数、哪个值错了）。
- `std::string_view`：轻量字符串"只读视图"，传参零拷贝（C++17）。
- `const char*`：C 风格字符串指针，老式 C 和现代 C++ 混用的典型。

### 3.5 `ParseArguments`（第 34–63 行）：逐个处理命令行参数

```cpp
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
```

**要点**（`argc` / `argv` 是 main 收到的命令行参数）：

- `argc`（argument count）：参数个数；`argv`（argument vector）：每个参数的内容。
- `argv[0]` 是**程序自己的名字**，所以循环从 `index = 1` 开始。
- 例如 `lqr_circle --radius 8 --duration 20`：
  - `argv[0]="lqr_circle"`，`argv[1]="--radius"`，`argv[2]="8"`，`argv[3]="--duration"`，`argv[4]="20"`
- 逻辑三步走：① 先看是不是 `--help`；② `index+1 >= argc` 检查**后面还有没有值**（防止 `--radius` 后面没数字）；③ `argv[++index]` **先自增再取值**，成对处理"参数名 + 参数值"。
- 长长的 `if/else if` 链：每个选项名对应设置 `options.simulation` 里的一个字段。简单直白，适合初学者；工业项目常用"参数表 + 循环匹配"。

### 3.6 `PrintHelp`（第 65–76 行）

```cpp
void PrintHelp() {
  std::cout << "2D point-mass circular trajectory tracking with discrete LQR\n\n"
            << "Usage: lqr_circle [options]\n"
            << "  --dt <seconds>       Sample time (default: 0.01)\n"
            // ... 其余选项说明 ...
            << "  -h, --help           Show this help\n";
}
```

- 纯输出文本，告诉用户有哪些选项和默认值。`\n` 是换行，`<<` 连续拼接。

### 3.7 `main` 函数（第 80–112 行）

```cpp
int main(int argc, char* argv[]) {
  try {
    // 1. 解析命令行参数
    const CommandLineOptions options = ParseArguments(argc, argv);

    // 2. 只要帮助 → 打印后正常退出
    if (options.show_help) {
      PrintHelp();
      return EXIT_SUCCESS;
    }

    // 3. 运行仿真（真正"干活"的地方在 lqr_sim 库内部）
    const lqr_sim::SimulationMetrics metrics =
        lqr_sim::RunSimulation(options.simulation, options.output);

    // 4. 格式化打印结果
    std::cout << std::fixed << std::setprecision(6)
              << "LQR DARE iterations : " << metrics.lqr.iterations << '\n'
              << "DARE residual       : " << metrics.lqr.residual << '\n'
              << "LQR gain K          : [" << metrics.lqr.gain(0,0) << ... << "]\n"
              << "Position RMSE       : " << metrics.position_rmse << " m\n"
              // ... 其余指标 ...
              << "CSV written to      : " << options.output.string() << '\n';
    return EXIT_SUCCESS;

  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
```

**要点**：

- **每个 C++ 程序必须有且只有一个 `main`**，程序从这里开始执行。
- `int` 返回值 = 退出码：`0` 成功，非 0 出错。
- 整个 main 包在 `try/catch` 里：任何一步抛异常（如参数写错），统一跳到 catch 打印错误并返回失败码。
- `const` 声明：解析完成后参数不再变化，防止误改。
- `lqr_sim::RunSimulation(...)`：命名空间前缀，调用 lqr_sim 库里的函数。真正的仿真循环在 `simulation.cpp`。
- `std::fixed << std::setprecision(6)`：固定小数位、保留 6 位，避免科学计数法。
- `std::cerr` 是错误流，与正常输出 `std::cout` 分开，脚本里可分别捕获。
- `EXIT_SUCCESS` = 0，`EXIT_FAILURE` = 1（来自 `<cstdlib>`）。

### 3.8 完整注释版源码

```cpp
// ===== 头文件：引入外部工具 =====
#include <cstdlib>      // EXIT_SUCCESS / EXIT_FAILURE
#include <filesystem>   // std::filesystem::path
#include <iomanip>      // std::fixed / std::setprecision
#include <iostream>     // std::cout / std::cerr
#include <stdexcept>    // std::invalid_argument
#include <string>       // std::string
#include <string_view>  // std::string_view
#include "lqr_sim/simulation.hpp"  // 项目接口：SimulationConfig / SimulationMetrics / RunSimulation

namespace {  // 匿名命名空间：以下内容仅本文件可见

// 程序所有可配置项打包成一个结构体
struct CommandLineOptions {
  lqr_sim::SimulationConfig simulation;              // 仿真参数
  std::filesystem::path output{"results/trajectory.csv"};  // 输出路径
  bool show_help{false};                             // 是否只显示帮助
};

// 把字符串安全地解析成 double，失败时抛带上下文的异常
double ParseDouble(std::string_view option, const char* value) {
  try {
    std::size_t parsed = 0;
    const double result = std::stod(std::string(value), &parsed);
    if (parsed != std::string(value).size()) {
      throw std::invalid_argument("trailing characters");
    }
    return result;
  } catch (const std::exception&) {
    throw std::invalid_argument("Invalid value for " + std::string(option) +
                                ": " + value);
  }
}

// 遍历 argv，把 "--xxx value" 逐对解析进 options
CommandLineOptions ParseArguments(int argc, char* argv[]) {
  CommandLineOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "--help" || argument == "-h") {
      options.show_help = true;
      continue;
    }
    if (index + 1 >= argc) {  // 后面没有值
      throw std::invalid_argument("Missing value for " + std::string(argument));
    }
    const char* value = argv[++index];
    if (argument == "--dt")            options.simulation.sample_time      = ParseDouble(argument, value);
    else if (argument == "--duration") options.simulation.duration         = ParseDouble(argument, value);
    else if (argument == "--radius")   options.simulation.radius           = ParseDouble(argument, value);
    else if (argument == "--omega")    options.simulation.angular_rate     = ParseDouble(argument, value);
    else if (argument == "--max-accel")options.simulation.max_acceleration = ParseDouble(argument, value);
    else if (argument == "--output")   options.output = value;
    else throw std::invalid_argument("Unknown option: " + std::string(argument));
  }
  return options;
}

// 打印使用帮助
void PrintHelp() { /* std::cout << 帮助文本 ... */ }

}  // namespace

// ===== 程序入口 =====
int main(int argc, char* argv[]) {
  try {
    const CommandLineOptions options = ParseArguments(argc, argv);  // ① 解析参数
    if (options.show_help) { PrintHelp(); return EXIT_SUCCESS; }    // ② 帮助模式

    const lqr_sim::SimulationMetrics metrics =                       // ③ 跑仿真
        lqr_sim::RunSimulation(options.simulation, options.output);

    std::cout << std::fixed << std::setprecision(6)                  // ④ 打印结果
              << "LQR DARE iterations : " << metrics.lqr.iterations << '\n'
              << "DARE residual       : " << metrics.lqr.residual << '\n'
              << "LQR gain K          : [" << metrics.lqr.gain(0,0) << ", "
                                          << metrics.lqr.gain(0,1) << ", "
                                          << metrics.lqr.gain(0,2) << ", "
                                          << metrics.lqr.gain(0,3) << "]\n"
              << "                      [" << metrics.lqr.gain(1,0) << ", "
                                          << metrics.lqr.gain(1,1) << ", "
                                          << metrics.lqr.gain(1,2) << ", "
                                          << metrics.lqr.gain(1,3) << "]\n"
              << "Position RMSE       : " << metrics.position_rmse << " m\n"
              << "Steady-state RMSE   : " << metrics.steady_state_rmse << " m\n"
              << "Maximum error       : " << metrics.maximum_position_error << " m\n"
              << "Final error         : " << metrics.final_position_error << " m\n"
              << "CSV written to      : " << options.output.string() << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {          // ⑤ 统一错误处理
    std::cerr << "Error: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
```

---

## 四、从 main.cpp 到 lqr_circle.exe

### 4.1 两个世界：源码 vs 可执行文件

| | `src/main.cpp`（源码） | `build/lqr_circle.exe`（可执行文件） |
|---|---|---|
| 本质 | 纯**文本** | **二进制**机器码 |
| 写给谁看 | 人 + 编译器 | CPU |
| 内容 | `if`、`for`、`std::cout` 等 | 一串 0/1 机器指令 |
| 能直接运行吗 | 不能 | 能 |

**核心**：CPU 看不懂高级语言，需要编译器（g++）把源码"翻译"成机器码。

打个比方：`main.cpp` 是中文菜谱，CPU 是只懂机器语的外国厨师，**g++ 是翻译官**，翻译好的成品就是 exe。

### 4.2 名字从哪来：`lqr_circle` 不是魔法

exe 的名字是**开发者自己指定的**，与"main"无关：

- `CMakeLists.txt` 第 21 行：`add_executable(lqr_circle src/main.cpp)` —— 程序名叫 `lqr_circle`，源文件是 `main.cpp`
- `build.ps1` 第 27 行：`g++ ... -o build/lqr_circle.exe` —— `-o` 指定输出文件名

`main.cpp` 只是恰好包含 `main` 函数的那个源文件。**运行 exe 时 `argv[0]` 拿到的就是 exe 自己的名字**——这就是为什么 main 里循环从 `index = 1` 开始。

### 4.3 四阶段流水线

```
源码 .cpp / .hpp
   │
   ▼  ① 预处理 (Preprocess)   -E
   │      #include 展开、#define 替换
   ▼  ② 编译 (Compile)        -S
   │      高级语言 → 汇编语言（仍接近可读）
   ▼  ③ 汇编 (Assemble)       -c
   │      汇编 → 机器码（.o 目标文件）
   ▼  ④ 链接 (Link)
   │      多个 .o 拼成完整程序，填上互相调用的地址
   ▼
lqr_circle.exe  ← 可被 CPU 执行的完整程序
```

**链接为什么必要**：`main.cpp` 调用了 `RunSimulation`，但它只知道"有这么个函数"，不知道函数机器码在哪。链接器负责把 main 的目标码和 simulation.cpp 的目标码**拼起来并对上地址**，还要补程序启动代码（操作系统先执行启动代码，再跳进你的 `main`）。

**本项目有 5 个源文件**（build.ps1 第 19–24 行 + main.cpp），每个单独翻译成一份目标码，最后拼成一个 exe：

```
circle_trajectory.cpp ─┐
lqr_controller.cpp   ─┤
point_mass.cpp       ─┤──→ 各自变成 .o ──→ 链接器 ──→ lqr_circle.exe
simulation.cpp       ─┤
main.cpp             ─┘
```

---

## 五、亲手实验记录

> 环境：Windows + MinGW g++ 15.2.0。以下命令均在项目根目录执行。

### 5.1 四阶段命令

```powershell
# 阶段① 预处理：把 #include 全部展开（产物是文本）
g++ -std=c++17 -I include -E src/main.cpp -o build/stages/main.i

# 阶段② 编译到汇编：高级语言 → 汇编语言（文本）
g++ -std=c++17 -I include -S src/main.cpp -o build/stages/main.s

# 阶段③ 汇编到目标文件：汇编 → 二进制机器码
g++ -std=c++17 -I include -c src/main.cpp -o build/stages/main.o

# 阶段④ 链接：5 个源文件 + 启动代码拼成完整 exe
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include `
    src/circle_trajectory.cpp src/lqr_controller.cpp src/point_mass.cpp `
    src/simulation.cpp src/main.cpp -o build/lqr_circle.exe
```

### 5.2 产物对比表（实测数据）

| 阶段 | 产物 | 大小 | 行数 | 内容 | 能运行吗 |
|---|---|---|---|---|---|
| 源码 | `src/main.cpp` | **5 KB** | 112 | C++ 源码文本 | ❌ |
| ① 预处理 | `build/stages/main.i` | **1860 KB** | 48,076 | `#include` 展开后的完整文本 | ❌ |
| ② 编译 | `build/stages/main.s` | **298 KB** | 10,939 | 汇编语言 | ❌ |
| ③ 汇编 | `build/stages/main.o` | **221 KB** | — | 二进制机器码（地址有洞） | ❌ |
| ④ 链接 | `build/lqr_circle.exe` | **133 KB** | — | 完整可执行程序 | ✅ |

### 5.3 亲眼验证的三个关键发现

**发现 1：`#include` 真的是"复制粘贴"**
`main.i` 第 6 行起就是 `D:/mingw64/lib/gcc/.../cstdlib` 的原文。112 行的 main.cpp 预处理后变成 48,076 行——这就是"翻译官先读完整份菜谱"。

**发现 2：函数名会被"加密"（名字修饰 name mangling）**
在 `main.s` 里能看到：

```asm
call _ZN12_GLOBAL__N_1L14ParseArgumentsEiPPc
call _ZN7lqr_sim13RunSimulationERKNS_16SimulationConfigERKNSt10filesystem7__cxx114pathE
```

`_ZN12_GLOBAL__N_1L14ParseArgumentsEiPPc` 就是把 `匿名命名空间::ParseArguments(int, char**)` 编码后的结果。C++ 用它区分同名重载函数。

**发现 3：main.o 单独不能运行的直接证据**
反汇编 `main.o`，对 `RunSimulation` 的调用机器码是：

```asm
ace: e8 00 00 00 00        call   ad3 <main+0x78>
```

`e8 00 00 00 00` = "一条**还没填地址**的 call 指令"（`00 00 00 00` 就是留的洞）。因为 `RunSimulation` 在 `simulation.cpp` 里，main.o 编译时不知道它的地址。**链接器的工作就是把这些洞填上**。同样，对 `std::cout`、`std::fixed`、`__main` 的引用也都是待填的洞。

**附加观察**：exe（133 KB）比 main.o（221 KB）还小——因为没开调试信息，且 main.o 里有很多模板代码"半成品"（COMDAT 段），链接时重复的被丢弃只留一份。

### 5.4 运行验证

```powershell
# 帮助模式
.\build\lqr_circle.exe --help

# 跑一个 3 秒仿真
.\build\lqr_circle.exe --duration 3 --output results/demo.csv

# 查看生成的 CSV
Get-Content results/demo.csv -TotalCount 6
```

CSV 表头：`time,ref_x,ref_y,ref_vx,ref_vy,x,y,vx,vy,ax_cmd,ay_cmd,position_error`

---

## 六、运行方法

```powershell
# 一键构建 + 运行（项目自带脚本）
powershell -ExecutionPolicy Bypass -File .\build.ps1
powershell -ExecutionPolicy Bypass -File .\run.ps1

# 或手动
.\build\lqr_circle.exe                          # 默认参数
.\build\lqr_circle.exe --duration 20 --radius 8 --omega 0.3 --output results\custom.csv
.\build\lqr_circle.exe --help                   # 查看全部选项
```

**重要**：改完源码后必须**重新构建**（`build.ps1`），exe 才会更新——exe 是旧源码的编译产物。

---

## 七、知识点速查表

| 代码片段 | 知识点 |
|---|---|
| `#include <...>` / `#include "..."` | 引入标准库 / 项目头文件 |
| `namespace { ... }` | 匿名命名空间：文件内私有 |
| `struct X { ... };` | 结构体：打包相关变量 |
| `double field{0.01};` | 默认成员初始化 |
| `std::string_view` | 轻量字符串"只读视图"（C++17） |
| `try / catch / throw` | 异常处理：出错抛出去、统一处理 |
| `int argc, char* argv[]` | main 收到的命令行参数 |
| `argv[++index]` | 先自增再取值的技巧 |
| `const` 变量 | 只读变量，防止误改 |
| `std::fixed << std::setprecision(6)` | 控制浮点输出格式 |
| `EXIT_SUCCESS` / `EXIT_FAILURE` | 标准退出码（0 / 1） |
| `lqr_sim::RunSimulation(...)` | 调用别的命名空间里的函数 |
| `-E` / `-S` / `-c` / `-o` | g++ 各阶段开关 / 输出文件名 |
| 名字修饰（mangling） | 编译器把函数签名编码成唯一符号名 |
| 重定位（relocation） | 目标文件里"待填的地址洞"，由链接器填充 |

---

## 八、常见疑问 FAQ

**Q1：为什么运行 `lqr_circle.exe` 而不是 `main.cpp`？**
因为 CPU 只认机器码。`main.cpp` 是给人看的文本，必须经过 g++ 四步翻译。`lqr_circle` 是开发者起的程序名（CMakeLists.txt 里 `add_executable` 指定）。

**Q2：改了 main.cpp 后，运行 exe 没变化？**
因为 exe 是旧源码编译出来的。改完必须重新构建。

**Q3：同一个 main.cpp 能变成不同的 exe 吗？**
能。不同编译器、不同平台会得到不同的 exe。**源码可移植，exe 绑定平台**——Windows 上编译的 exe 拿到 Linux 跑不了。

**Q4：为什么记事本打开 exe 是乱码？**
因为它是二进制机器码，不是给人看的文本。源码给人看，exe 给机器跑。

**Q5：为什么 exe 比 main.o 还小？**
main.o 是"拼图碎片"（含大量模板半成品 COMDAT 段），链接时去重；exe 是去重后的成品，且 Release 模式没带调试符号。

**Q6：`main.cpp` 和 `main` 函数是什么关系？**
`main` 函数是程序入口（必须存在）；`main.cpp` 只是恰好包含它的源文件名。exe 里可能有任意多个源文件，但 `main` 函数只能有一个。

---

## 九、下一步学习建议

main.cpp 只是"外壳"。推荐阅读顺序（README 建议）：

```
main.cpp → simulation.cpp → point_mass.cpp → circle_trajectory.cpp → lqr_controller.cpp → matrix.hpp
```

- `simulation.cpp`：仿真主循环（for 循环逐帧更新状态）和 CSV 写入
- `point_mass.cpp`：被控对象（二维双积分质点）的离散更新
- `circle_trajectory.cpp`：圆轨迹解析参考（位置/速度/加速度）
- `lqr_controller.cpp`：LQR 核心（DARE 迭代求解 + 增益计算）
- `matrix.hpp`：定长矩阵、乘法、转置、Gauss-Jordan 求逆

**动手实验建议**：

1. 把 `--duration 3` 改成 `--duration 30`，看误差如何收敛。
2. 试试错误参数：`.\build\lqr_circle.exe --radius`（缺值）、`--abc 1`（未知选项），看 catch 如何报错。
3. 改 `CMakeLists.txt` 里的 `add_executable(lqr_circle ...)` 程序名，重新构建，观察 exe 名字变化。
4. 手动跑一遍四阶段命令（见 5.1），打开 `main.s` 找 `main:` 标签和 `RunSimulation` 的长名字。

---

## 十、实战练习题（面向实际工作技能）

> 这些练习模拟真实工作中会遇到的**任务类型**，不是语法填空题。难度按 入门 → 进阶 → 挑战 排列，每个练习都给了**验收标准**——做完能自己确认"我真的做对了"。
>
> 真实工作技能对应表：
>
> | 练习 | 对应的工作技能 |
> |---|---|
> | 10.1 | 需求开发：读接口 → 改代码 → 更新文档 → 验证 |
> | 10.2 | 验收测试、缺陷报告 |
> | 10.3 | gdb 调试（每天都会用） |
> | 10.4 | 构建系统、增量编译原理 |
> | 10.5 | 可测试性设计、重构、单元测试 |
> | 10.6 | 代码审查（Code Review） |
> | 10.7 | Git 分支工作流、规范 commit |
> | 10.8 | 可观测性：错误信息质量 |
> | 10.9 | 构建配置：Debug vs Release |

### 10.1 需求开发：给程序加一个新命令行选项（入门）

**背景**：产品经理说"用户想自定义小车的初始位置"。这就是一次真实的"加功能"需求。

**任务**：给程序增加 `--initial-x` 和 `--initial-y` 两个选项，设置仿真初始位置。

**提示**：

- `SimulationConfig` 里已经有 `initial_state` 字段（见 `simulation.hpp` 第 15 行：`State initial_state{7.0, -1.0, 0.0, 0.0};`，含义是 `[px, py, vx, vy]`）。
- `Matrix` 的 `operator()(row, col)` 可写（`matrix.hpp` 第 24 行返回 `double&`），所以可以 `options.simulation.initial_state(0, 0) = x;`
- 需要改三处：`ParseArguments` 里加两个 `else if` 分支；`PrintHelp` 里加两行说明；别忘了 `SimulationConfig` 是**传引用**给 `RunSimulation` 的，改的是同一个对象。

**验收标准**：

```powershell
.\build\lqr_circle.exe --initial-x 0 --initial-y 0 --duration 1 --output results/init.csv
Get-Content results/init.csv -TotalCount 2   # 第二行 x、y 应接近 0
```

CSV 第二行的 `x,y` 列应接近 0、0（而不是默认的 7、-1）。`--help` 里能看到新选项。

**加分项**：改完跑一遍 `build.ps1`，确认没有破坏现有测试（`lqr_tests.exe` 全部通过）——真实工作中这叫"回归验证"。

### 10.2 缺陷发现：非法输入测试（入门）

**背景**：真实工作中，你的代码会被各种"不按说明书操作"的用户使用。上线前要自己先当一回"坏用户"。

**任务**：对程序做一轮非法输入测试，用下面的表格记录每个用例的行为（注意 `$LASTEXITCODE`，0 = 成功退出，1 = 出错退出）：

| 输入 | 期望行为 | 实际行为 | 退出码 |
|---|---|---|---|
| `--radius`（缺值） | 报错退出 | | |
| `--abc 1`（未知选项） | 报错退出 | | |
| `--dt 0` | ？（程序没校验） | | |
| `--dt -1` | ？（程序没校验） | | |
| `--duration 0` | ？（程序没校验） | | |
| `--output Z:\不存在\目录\x.csv` | ？（看报错是否可理解） | | |

**任务第二部分**：把发现的问题写成一条**缺陷报告**，格式：`复现步骤 / 期望 / 实际 / 严重程度`。例如 `--dt 0` 如果程序能跑出结果或崩溃，就是一个"输入校验缺失"缺陷。

**验收标准**：能回答"程序目前对哪些输入缺少校验？这些校验应该加在 `ParseArguments` 里还是 `ParseDouble` 里？"（提示：考虑职责划分——解析格式 vs 语义合法）。

### 10.3 调试技能：用 gdb 观察参数解析（入门 → 进阶）

**背景**：真实工作中，"程序行为不符合预期"时第一反应不是加打印，而是**用调试器断点单步**。本项目环境已装好 gdb 17.1。

**任务**：用 gdb 观察 `--radius 8` 是怎么一路走进 `options.simulation.radius` 的。

**步骤**：

```powershell
# 1. 先构建 Debug 版（带调试信息、不优化）
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Debug

# 2. 启动 gdb
gdb --args .\build\lqr_circle.exe --radius 8 --duration 1

# 3. 在 gdb 交互界面里依次输入：
#    break ParseArguments      ← 打断点（可以补全函数名）
#    run                       ← 运行到断点
#    print argc                ← 看参数个数（应为 5）
#    print argv[1]             ← 看第一个参数
#    next                      ← 单步走几行
#    print options.simulation.radius  ← 走完 if 分支后看值（可能需要先 finish）
#    quit
```

**关键点**：为什么要用 Debug 版？因为 Release（`-O2`）会**重排代码、内联函数**，单步时你会看到"乱跳"甚至"跳不进函数"。真实工作中 Debug 用于开发调试，Release 用于交付。

**验收标准**：能画出一条数据流："`argv[3]`（字符串 `"8"`）→ `ParseDouble` 返回 `8.0` → `options.simulation.radius`"。试试在 `ParseDouble` 里也打断点，观察 `stod` 前后的值。

### 10.4 构建系统思维：手动增量编译（进阶）

**背景**：真实项目有几十上百个源文件，**不可能每次全量编译**。CMake/Make/ninja 存在的意义就是"只重编译改过的文件"。

**任务**：

```powershell
# 1. 手动把 5 个源文件分别编译成目标文件
g++ -std=c++17 -O2 -I include -c src/main.cpp -o build/stages/main.o
g++ -std=c++17 -O2 -I include -c src/simulation.cpp -o build/stages/simulation.o
g++ -std=c++17 -O2 -I include -c src/lqr_controller.cpp -o build/stages/lqr_controller.o
g++ -std=c++17 -O2 -I include -c src/point_mass.cpp -o build/stages/point_mass.o
g++ -std=c++17 -O2 -I include -c src/circle_trajectory.cpp -o build/stages/circle_trajectory.o

# 2. 计时"全量编译"（-c 全部 5 个文件）
Measure-Command { g++ -std=c++17 -O2 -I include -c src/main.cpp -o build/stages/main.o; ... }

# 3. 只改 main.cpp 的一处（比如帮助文本），只重新 -c main.cpp，再链接 5 个 .o：
g++ build/stages/*.o -o build/lqr_circle.exe
```

**验收标准**：能解释清楚：为什么"只重编 1 个文件 + 链接"比"全量编译"快？链接为什么必须每次都做？（回忆 5.3：main.o 里的地址洞靠链接填充；任何 .o 变了，exe 都要重新拼）。以及：`CMakeLists.txt` / `build.ps1` 分别对应这里的哪一步？

### 10.5 可测试性重构：让 main.cpp 里的函数可被测试（进阶）

**背景**：打开 `tests/test_main.cpp`——它只能测试 `lqr_sim` 库（矩阵、轨迹、闭环），**测不到** `ParseDouble` / `ParseArguments`，因为它们藏在 main.cpp 的匿名命名空间里。真实工作中这叫"**不可测试的代码**"，是需要重构的信号。

**任务 A（热身）**：仿照 `TestCircleReference` 的写法，给 `test_main.cpp` 加一个测试：圆轨迹在时间 `t = 2π/ω`（转完一整圈）时，位置应回到起点 `(r, 0)`。用 `ExpectNear` 断言（注意浮点误差，容差取 `1e-9` 左右）。

**任务 B（挑战，推荐）**：把 `ParseDouble` / `ParseArguments` 从 main.cpp **抽取**到一个独立模块 `include/lqr_sim/cli.hpp` + `src/cli.cpp`（命名空间可以就叫 `lqr_sim::cli`），main.cpp 改为调用它们；然后在 `test_main.cpp` 里 include `cli.hpp`，给 `ParseArguments` 写测试，例如：

- 传入 `{"prog", "--radius", "8"}`，断言 `result.simulation.radius == 8.0`
- 传入 `{"prog", "--radius"}`（缺值），断言抛出异常（用 `try/catch` 捕获并检查 `error.what()` 包含 "Missing value"）

**验收标准**：`build.ps1` 构建成功，`lqr_tests.exe` 全部通过，原有功能不变（`--radius 8` 行为与之前一致）。

> 这是最重要的一道题：真实工作中"为了能测试而重构"每天都在发生。做完你会理解为什么生产代码要把"解析逻辑"和"程序入口"分开。

### 10.6 代码审查：当一次 reviewer（进阶）

**背景**：真实工作中，你的代码要过同事的 review；你也要 review 别人的。下面是一份"待审查"的 main.cpp。

**任务**：以 reviewer 身份找出**至少 3 个**可改进点，每个写成"问题 + 理由 + 修改建议"三条。参考候选（也可以自己找）：

1. `ParseDouble` 里 `std::string(value)` 构造了**两次**（`text` 构造一次，catch 里 `+ std::string(option)` 又一次）——小问题，但真实代码里这种重复很常见。
2. `PrintHelp` 里硬编码默认值（`default: 0.01` 等），与 `simulation.hpp` 里的默认初始化**重复**——改一处忘另一处，文档就和实现不一致了。真实项目会用常量/单点定义。
3. `dt`、`duration`、`radius` 没有**语义校验**：负数、零都能传进来（结合 10.2 的发现）。
4. `ParseArguments` 的 `if/else if` 链很长（7 个分支），可读性一般——真实项目可能用"选项表"（struct + 数组 + 循环）。

**任务第二部分**：挑其中一个改进点，**实际动手修掉**，重新构建并验证行为不变。

**验收标准**：交出一份 review 意见表（问题/理由/建议三列），并至少落地一个修改。

### 10.7 Git 工作流：模拟真实开发流程（进阶）

**背景**：本项目已经是 git 仓库。真实工作中改代码的标准流程是：**分支 → 开发 → 检查 diff → 提交（规范 message）→ 合并**。

**任务**：

```powershell
git status                        # 1. 看当前工作区状态
git log --oneline                 # 2. 看提交历史
git checkout -b feature/initial-position   # 3. 开功能分支
# 4. 在分支上完成 10.1 的 --initial-x/--initial-y 功能
git diff                          # 5. 审查自己的改动
git add src/main.cpp
git commit -m "feat(cli): add --initial-x/--initial-y options"
git log --oneline                 # 6. 确认提交
```

**验收标准**：`git log --oneline` 里能看到一条清晰描述这次改动的提交；`git show HEAD` 能展示完整的改动内容。

**加分项**：用 `git diff` 模拟"提交前自我审查"——真实工作里提交前要自己先看一遍 diff，确认没有混入无关改动（比如调试用的临时打印）。

### 10.8 可观测性：让错误信息可操作（挑战）

**背景**：真实工作中，用户（可能是同事、运维、客户）看到 `Error: xxx` 时最想知道的三个问题：**哪里错了？为什么？怎么办？** 好的错误信息要回答第三个问题。

**任务**：

1. 先运行 `.\build\lqr_circle.exe --output Z:\不存在的目录\x.csv`，观察当前报错——大概率只有一个晦涩的 `filesystem_error` 信息。
2. 改进：在 `main` 里调用 `RunSimulation` **之前**，用 `std::filesystem` 检查输出路径的父目录是否存在，不存在就抛出一个**带路径 + 建议动作**的友好错误。

**验收标准**：错误信息包含三要素：出错的路径、可能的原因（"父目录不存在"）、建议动作（"请先创建目录或检查 --output 参数"）。

**扩展思考**：真实系统里错误信息还要考虑**谁来读**（终端用户 vs 日志系统 vs 监控告警），格式和详略完全不同——这是可观测性（observability）的入门话题。

### 10.9 构建配置：Debug vs Release（挑战）

**背景**：交付给用户的是 Release 版，自己调试用 Debug 版。理解优化级别的影响是基本功。

**任务**：

```powershell
# 对比构建
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Debug
(Get-Item build/lqr_circle.exe).Length          # Debug exe 大小
Measure-Command { .\build\lqr_circle.exe --duration 30 }   # Debug 耗时

powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Release
(Get-Item build/lqr_circle.exe).Length          # Release exe 大小
Measure-Command { .\build\lqr_circle.exe --duration 30 }   # Release 耗时
```

**验收标准**：能解释三个问题：为什么 Release exe 更小？（-O2 去掉了调试符号并优化了代码）为什么通常更快？为什么调试时不用 Release？（结合 10.3 的知识）。

**加分项**：试试 `g++ -O3`，看和 `-O2` 比还有没有可感知的差异；再用 `objdump -d` 对比 Debug/Release 版 `main` 的汇编长度，直观感受优化做了什么。
