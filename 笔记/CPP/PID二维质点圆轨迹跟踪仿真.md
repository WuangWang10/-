# 二维质点跟踪圆轨迹的PID控制仿真_Cpp版

## 环境描述

对于位于坐标原点的质点$m = 1 kg$，通过PID控制器，使其跟踪半径为$5 m$的圆形轨迹$x_d = 5cos(\frac{\pi}{5}t),\quad y_d = 5sin(\frac{\pi}{5}t)$。

PID控制器的输出直接以加速度的形式作用在质点上，位置、速度和加速度之间的微积分转化采用数值求导/加和的方式实现。

最终将仿真结果以csv文件输出到同路径的文件夹中。

## 核心组成

该仿真的核心组成包括以下三者：

1. 受控对象状态

2. 受控对象动力学模型

3. 控制器

### 1. 受控对象状态

本仿真中采用`struct`的方式定义质点的状态。当受控对象模型更加复杂或代码要求严谨时可以使用`class`来定义受控对象，并明确`privite`和`public`权限范围。

定义质点模型状态如下：

1. 声明结构体

```cpp
struct State
{
    double x, y;    // 确定位置
    double vx, vy;  // 确定速度  
};
```

2. 创建结构体

```cpp
State state{0.0, 0.0, 0.0, 0.0};
```

### 2. 受控对象动力学模型

由于质点模型足够简单$m = 1 kg$，直接满足牛顿第二定律$F = ma \to F = a$，所以我们通过PID控制器求解得到的控制量（力）可以直接作为加速度输入质点的**位置—速度—加速度**积分中

```cpp
// x方向
state.vx += ux * dt;
state.x += state.vx * dt;

// y方向
state.vy = uy * dt;
state.y = state.vy * dt;
```

### 3. 控制器

与受控对象相同，PID控制器也使用`struct`来表示。

控制器声明中需要包含以下信息：

1. 控制器的kp,ki,kd参数

2. 积分限幅

3. 初始化积分项

4. 计算控制量

```cpp
struct PID
{
    double kp, ki, kd;
    double integralLimit;
    double integral = 0.0;
    
    double contrloLaw(double positionError, double velocityError, double dt)
    {
        integral += velocityError * dt;
        integral = std::clamp(integral, -integralLimit, integralLimit);
        
        return kp * positionError + ki * integral + kd * velocityError;
    }
};
```

在仿真主函数中创建该控制器：

```cpp
// x方向
PID pidX{5.0, 1.0, 0.01, 8.0};
// y方向
PID pidY{5.0, 1.0, 0.01, 8.0};
```

## 仿真主程序(循环)











---



```cpp
// 引用头文件
#include <algorithm>    // 后续用到的`std::clamp()`限幅来自该头文件
#include <cmath>        // 后续用到的`std::sin(),std::cos(),std::hypot()`来自该头文件
#include <fstream>      // 文件操作需要的头文件
#include <iostream>     // 标准输入输出流

// 定义常数PI
constexpr double PI = 3.14159265358979323846;
// Question:`constexpr`是什么关键字
// Answer: `constexpr`是表示"这个值可以在编译期间确定，并且之后不能修改"的关键字，PI是"编译期常量"。


// 声明质点模型状态结构体
// 状态包含两个维度的位置和速度
struct State 
{

    double x = 0.0, y = 0.0;
    double vx = 0.0, vy = 0.0;
};

// 声明PID控制器结构体类型
// 包括PID参数、积分限幅和PID控制量计算
struct PID 
{
    double kp, ki, kd;
    double integralLimit;   // 设置积分限幅
    double integral = 0.0;  // 初始化积分项

    // 计算tau = kp*error + ki*integral + kd*de
    // 函数输入 e, de, dt
    // 其中integral = integral + e*dt;
    double update(double positionError, double velocityError, double dt) 
    {
        integral += positionError * dt;                                     // 求解积分项
        integral = std::clamp(integral, -integralLimit, integralLimit);     // 积分限幅

        return kp * positionError + ki * integral + kd * velocityError;     // 计算控制量
    }
};
// Question:为什么用`struct`不用`class`

int main()
{
    constexpr double dt = 0.001;
    constexpr double duration = 20.0;
    constexpr double maxAcceleration = 20.0;


    constexpr double centerX = 0.0;
    constexpr double centerY = 0.0;
    constexpr double radius = 5.0;
    constexpr double omega = 2.0 * PI / 10.0;

    State state{-1.0, 0.0, 0.0, 0.0};
    PID pidX{8.0, 0.2, 5.0, 5.0};
    PID pidY{8.0, 0.2, 5.0, 5.0};

    std::ofstream file("trajectory.csv");
    file << "time,x,y,x_ref,y_ref,ux,uy,error\n";
    // Question:cpp文件操作语句

    for (double t = 0.0; t <= duration; t += dt)
    {
        const double theta = omega * t;

        const double xr = centerX + radius * std::cos(theta);
        const double yr = centerY + radius * std::sin(theta);

        const double vxr = -radius * omega * std::sin(theta);
        const double vyr = radius * omega * std::cos(theta);

        const double axr = -radius * omega * omega * std::cos(theta);
        const double ayr = -radius * omega * omega * std::sin(theta);

        const double ex = xr - state.x;
        const double ey = yr - state.y;

        double ux = axr + pidX.update(ex, vxr - state.vx, dt);
        double uy = ayr + pidY.update(ey, vyr - state.vy, dt);
        // 这里不用常数是因为后续需要判断更新

        const double acceleration = std::hypot(ux, uy);
        // Question:什么是`hypot()`
        // Answer: `hypot()`用于计算两个数的平方和

        if (acceleration > maxAcceleration) {
            const double scale = maxAcceleration / acceleration;
            ux *= scale;
            uy *= scale;
        }

        // 写入文件
        file << t << "," << state.x << "," << state.y << "," << xr << "," << yr << "," << ux << "," << uy << "," << std::hypot(ex, ey) << "\n";

        // 更新系统状态
        state.vx += ux * dt;
        state.vy += uy * dt;
        state.x += state.vx * dt;
        state.y += state.vy * dt;
    }

    std::cout << "Simulation finished: trajectory.csv\n";


    return 0;

}
```