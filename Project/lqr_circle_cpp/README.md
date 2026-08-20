# 二维质点圆轨迹 LQR 仿真

这是一个用于学习现代 C++、离散 LQR 和飞控外环基本结构的小型工程。被控对象是二维双积分质点，控制器输出 x/y 方向的加速度指令，跟踪器使用圆轨迹的位置、速度和加速度参考。

工程不依赖 Eigen 或测试框架。定长矩阵、DARE 迭代和测试均保留在项目内，目的是让算法路径透明。实际工程部署时应优先采用经过验证的线性代数库。

## 快速开始

当前 Windows/MinGW 环境可以直接运行：

```powershell
cd <本工程目录>
powershell -ExecutionPolicy Bypass -File .\build.ps1
powershell -ExecutionPolicy Bypass -File .\run.ps1
```

运行后会生成：

- `results/trajectory.csv`：完整时序数据，适合 MATLAB、Python 或表格软件分析。
- `results/trajectory.svg`：参考轨迹与实际轨迹图；绘图只使用 Python 标准库。

也可以直接修改参数：

```powershell
.\build\lqr_circle.exe --duration 20 --radius 8 --omega 0.3 `
  --max-accel 6 --output results\custom.csv
python .\tools\plot_results.py results\custom.csv results\custom.svg
```

如果系统已安装 CMake：

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

## 数学模型

状态和控制输入定义为：

```text
x = [px, py, vx, vy]^T
u = [ax, ay]^T
```

假设一个采样周期内加速度恒定，使用零阶保持精确离散化：

```text
x(k+1) = A x(k) + B u(k)

    [1  0  dt  0 ]       [dt^2/2     0   ]
A = [0  1  0   dt]   B = [   0     dt^2/2]
    [0  0  1    0]       [  dt       0   ]
    [0  0  0    1]       [   0      dt   ]
```

圆轨迹解析参考为：

```text
p_ref = [r cos(wt), r sin(wt)]^T
v_ref = [-rw sin(wt), rw cos(wt)]^T
a_ref = [-rw^2 cos(wt), -rw^2 sin(wt)]^T
```

控制器使用“前馈 + 反馈”结构：

```text
u = a_ref - K (x - x_ref)
```

`a_ref` 提供维持圆周运动所需的向心加速度。没有它时，LQR 仍能跟踪，但会持续用状态误差产生向心加速度，通常留下更大的相位或位置误差。

## 离散 LQR

性能指标为：

```text
J = sum(x_error^T Q x_error + u_error^T R u_error)
```

程序迭代求解离散代数 Riccati 方程（DARE）：

```text
P_next = A^T P A - A^T P B (R + B^T P B)^-1 B^T P A + Q
K      = (R + B^T P B)^-1 B^T P A
```

默认权重为：

```text
Q = diag(20, 20, 5, 5)
R = diag(1, 1)
```

- 增大 `Q(位置,位置)`：位置误差代价更高，响应更激进。
- 增大 `Q(速度,速度)`：增加速度误差阻尼，通常减小超调。
- 增大 `R`：控制动作更昂贵，响应更平缓。
- 权重没有绝对的“最佳值”，应结合状态单位、执行器限制、噪声和期望带宽选择。

加速度指令还受到向量模长限制，而不是分别裁剪 x/y 分量。这样不会因为对角方向运动而得到更大的总加速度。

## 代码结构

```text
include/lqr_sim/
  matrix.hpp             定长矩阵、乘法、转置、Gauss-Jordan 求逆
  types.hpp              状态、输入及矩阵类型别名
  point_mass.hpp         二维质点接口
  circle_trajectory.hpp  圆轨迹参考生成器
  lqr_controller.hpp     DARE 求解和控制律
  simulation.hpp         仿真配置与指标
src/
  point_mass.cpp         离散对象更新
  circle_trajectory.cpp  解析位置/速度/加速度参考
  lqr_controller.cpp     LQR 核心实现
  simulation.cpp         固定周期仿真循环和 CSV 记录
  main.cpp               命令行解析与结果打印
tests/test_main.cpp       矩阵、轨迹与闭环跟踪测试
tools/plot_results.py     CSV 到 SVG 的无依赖绘图工具
```

建议按以下顺序阅读：`main.cpp` -> `simulation.cpp` -> `point_mass.cpp` -> `circle_trajectory.cpp` -> `lqr_controller.cpp` -> `matrix.hpp`。

## 与 PX4 学习的衔接

这个示例对应的是一个理想化的位置外环：位置/速度误差经过反馈后形成期望加速度。迁移到多旋翼时，期望水平加速度还需要转换为期望推力方向和姿态，再由姿态角速度内环驱动电机。

继续学习时需要特别关注：

1. **坐标系**：本工程使用普通 x-y 平面；PX4 常见 NED 和机体系，z 轴方向及旋转定义必须统一。
2. **离散周期**：`dt` 同时影响对象矩阵和 LQR 增益。飞控循环频率变化后必须重新离散化和求解。
3. **状态估计**：本工程直接读取真值；真实飞控使用 EKF 输出，存在噪声、延迟和跳变。
4. **级联结构**：这里把加速度直接施加到质点；真实飞行器还包含姿态、角速度、电机和空气动力学动态。
5. **约束与抗饱和**：真实系统有倾角、推力、速度和加加速度限制。LQR 本身不是约束优化器。
6. **安全验证**：先做软件在环和硬件在环，再做有保护条件的实机测试；不能把本示例直接刷入飞控实飞。

## 推荐实验

1. 将圆轨迹前馈加速度设为零，比较稳态 RMSE 和相位滞后。
2. 分别把位置权重改为 `2`、`20`、`100`，观察初始收敛和加速度峰值。
3. 把最大加速度限制降到 `1 m/s^2`，同时增大圆半径或角速度，观察物理不可跟踪条件 `r*w^2 > a_max`。
4. 给状态加入高斯噪声，体会 LQR 状态反馈与 LQG（LQR + Kalman Filter）的关系。
5. 扩展为 `[x,y,z,vx,vy,vz]`，再加入姿态/推力映射，逐步接近多旋翼级联控制结构。

## 实现边界

轻量矩阵求逆采用带主元选择的 Gauss-Jordan 法，足以覆盖这里的 2x2 正定矩阵。生产飞控代码应使用经过充分测试、数值条件明确且适合嵌入式平台的矩阵库，并对可控性、权重正定性、求解失败和浮点异常做完整处理。
