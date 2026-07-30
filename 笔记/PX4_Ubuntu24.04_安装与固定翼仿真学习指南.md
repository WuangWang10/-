# PX4 在 Ubuntu 24.04（WSL2）中的安装与固定翼仿真学习指南

> 适用目标：在 Windows 11 的 WSL2 Ubuntu 24.04 中安装 PX4，使用 Gazebo Harmonic 运行固定翼 SITL 仿真，并为后续控制算法验证准备开发环境。
>
> 本文中的 Windows 混合代理端口为 `7897`。

## 1. 先分清两个命令行环境

安装和编译 PX4 的绝大多数命令都在 **Ubuntu 终端**运行。

| 提示符示例 | 环境 | 主要用途 |
|---|---|---|
| `wmj@DESKTOP-781F6CO:~$` | Ubuntu / WSL2 | Git、PX4 安装、编译、Gazebo 仿真 |
| `PS C:\Users\lenovo>` | Windows PowerShell | 启动、关闭和检查 WSL |

PowerShell 中常用的 WSL 管理命令：

```powershell
# 查看已安装的 WSL 发行版及版本
wsl -l -v

# 启动 Ubuntu 24.04
wsl -d Ubuntu-24.04

# 彻底关闭全部 WSL 实例
wsl --shutdown
```

以下未特别标注的命令均在 **Ubuntu 终端**运行。

## 2. 检查 Ubuntu 环境

```bash
lsb_release -a
uname -m
```

预期结果：

- Ubuntu 版本为 `24.04`；
- CPU 架构为 `x86_64`。

更新软件索引并确保 Git 已安装：

```bash
sudo apt update
sudo apt install -y git
```

命令解释：

- `sudo`：以管理员权限执行命令；
- `apt update`：刷新 Ubuntu 软件包索引；
- `apt install -y git`：安装 Git，`-y` 表示自动确认安装。

## 3. 配置 Windows 代理供 WSL 使用

### 3.1 为什么需要单独配置

Windows 代理软件监听的 `7897` 是混合代理端口。WSL2 是独立的 Linux 网络环境，Windows 中已经启用代理，不代表 Ubuntu 中的 Git、`pip` 和 `curl` 会自动使用它。

如果 Windows 代理软件支持，请确认：

- 已开启 `Allow LAN`（允许局域网连接）；
- 监听地址为 `0.0.0.0`，而不只是 Windows 的 `127.0.0.1`；
- Windows 防火墙允许该代理程序访问专用网络。

### 3.2 测试 localhost 方式

较新的 WSL 镜像网络模式可能可以直接访问 Windows 的 localhost：

```bash
curl -x http://127.0.0.1:7897 \
  -I --connect-timeout 15 \
  https://github.com
```

参数解释：

- `-x`：指定代理地址；
- `-I`：只请求 HTTP 响应头，不下载网页正文；
- `--connect-timeout 15`：连接超时时间为 15 秒。

如果返回 `HTTP/2 200` 或 `HTTP/1.1 200`，说明可直接使用 `127.0.0.1:7897`。

### 3.3 测试 WSL 网关方式

如果 localhost 方式失败，获取 Windows 主机在 WSL NAT 网络中的地址：

```bash
WIN_HOST=$(ip route show | awk '/default/ {print $3; exit}')
echo "$WIN_HOST"
```

命令解释：

- `ip route show`：显示 Linux 路由表；
- `awk ...`：提取默认网关地址；
- `$(...)`：执行括号内命令，并把输出赋给变量 `WIN_HOST`。

测试代理：

```bash
curl -x "http://${WIN_HOST}:7897" \
  -I --connect-timeout 15 \
  https://github.com

curl -x "http://${WIN_HOST}:7897" \
  -I --connect-timeout 15 \
  https://pypi.org/simple/cerberus/
```

GitHub 和 PyPI 都返回成功状态后，再继续安装。

### 3.4 设置当前 Ubuntu 终端的代理

如果使用 WSL 网关地址：

```bash
WIN_HOST=$(ip route show | awk '/default/ {print $3; exit}')

export http_proxy="http://${WIN_HOST}:7897"
export https_proxy="http://${WIN_HOST}:7897"
export HTTP_PROXY="$http_proxy"
export HTTPS_PROXY="$https_proxy"
export no_proxy="localhost,127.0.0.1,::1"
export NO_PROXY="$no_proxy"
```

如果 `127.0.0.1:7897` 测试成功，则改用：

```bash
export http_proxy="http://127.0.0.1:7897"
export https_proxy="http://127.0.0.1:7897"
export HTTP_PROXY="$http_proxy"
export HTTPS_PROXY="$https_proxy"
export no_proxy="localhost,127.0.0.1,::1"
export NO_PROXY="$no_proxy"
```

`export` 设置只对当前终端及其子进程生效，因此不会永久污染系统配置。打开新终端后需要重新执行。

查看当前代理环境变量：

```bash
env | grep -i proxy
```

关闭当前终端的代理：

```bash
unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY
unset no_proxy NO_PROXY
```

### 3.5 Git 专用代理（可选）

仅对一条 Git 命令使用代理：

```bash
git \
  -c http.proxy="http://${WIN_HOST}:7897" \
  -c https.proxy="http://${WIN_HOST}:7897" \
  ls-remote https://github.com/PX4/PX4-Autopilot.git HEAD
```

这里的 `-c` 是临时 Git 配置，只对当前命令有效，不会让后续的 `pip` 或安装脚本使用代理。

如需设置 Git 全局代理：

```bash
git config --global http.proxy "http://${WIN_HOST}:7897"
git config --global https.proxy "http://${WIN_HOST}:7897"
```

取消 Git 全局代理：

```bash
git config --global --unset http.proxy
git config --global --unset https.proxy
```

由于 WSL 网关地址可能在重启后改变，优先推荐会话级环境变量或单条命令的 `-c` 配置。

## 4. 下载 PX4 源码

PX4 仓库及子模块较大，完整克隆容易发生：

```text
RPC failed; curl 56 Recv failure: Connection reset by peer
fatal: early EOF
fatal: fetch-pack: invalid index-pack output
```

这通常是大数据传输中途断开，不是 PX4 源码错误。建议使用浅克隆并降低并发。

### 4.1 处理失败残留目录

```bash
cd ~

[ -e PX4-Autopilot ] && \
  mv PX4-Autopilot "PX4-Autopilot.failed.$(date +%s)"
```

这会把旧目录改名保留，而不是直接删除。`date +%s` 会生成时间戳，防止覆盖已有备份。

### 4.2 降低 Git 网络并发

```bash
git config --global http.version HTTP/1.1
git config --global http.maxRequests 1
```

命令解释：

- 强制 Git 使用 HTTP/1.1，可减少部分 HTTP/2 长连接重置问题；
- `http.maxRequests 1` 将并行 HTTP 请求数降为 1，更适合不稳定网络。

### 4.3 浅克隆主仓库

确保代理环境变量已经设置，然后执行：

```bash
cd ~

git clone \
  --depth 1 \
  --single-branch \
  --filter=blob:none \
  https://github.com/PX4/PX4-Autopilot.git
```

参数解释：

- `--depth 1`：只下载最新一次提交所需的浅历史；
- `--single-branch`：只获取当前默认分支；
- `--filter=blob:none`：不下载当前工作区不需要的历史文件内容；
- 默认分支为 `main`，适合使用最新 PX4 开发版本。

### 4.4 低并发下载全部子模块

```bash
cd ~/PX4-Autopilot

git submodule update \
  --init \
  --recursive \
  --depth 1 \
  --jobs 1
```

参数解释：

- `--init`：初始化尚未初始化的子模块；
- `--recursive`：继续处理子模块内部嵌套的子模块；
- `--depth 1`：对子模块也使用浅克隆；
- `--jobs 1`：一次只下载一个子模块。

如果中途断开，直接重复这条命令即可。已经完成的子模块不会重新下载。

## 5. 验证源码和子模块

```bash
cd ~/PX4-Autopilot
git status
git submodule status --recursive
```

正确状态应包括：

```text
On branch main
Your branch is up to date with 'origin/main'.
nothing to commit, working tree clean
```

`git submodule status` 每行提交号前面的符号含义：

| 前缀 | 含义 |
|---|---|
| 空格 | 子模块已初始化，版本正确 |
| `-` | 子模块尚未初始化 |
| `+` | 子模块当前提交与主仓库记录不一致 |
| `U` | 子模块存在合并冲突 |

所有行以空格开头即为正常。

## 6. 安装 PX4 开发环境

确保当前终端已经配置代理，然后执行：

```bash
cd ~/PX4-Autopilot
bash Tools/setup/ubuntu.sh
```

命令解释：

- `bash`：使用 Bash 执行安装脚本；
- 脚本会安装编译器、CMake、Python 依赖、Gazebo Harmonic，以及 PX4 所需工具链；
- 脚本内部需要管理员操作时会自行调用 `sudo`，不要使用 `sudo bash Tools/setup/ubuntu.sh`；
- 脚本可以重复运行，已安装的软件包会自动跳过。

如果确定只做 SITL 仿真、不编译 Pixhawk/NuttX 固件，可使用：

```bash
bash Tools/setup/ubuntu.sh --no-nuttx
```

如果未来可能进行硬件在环或实机部署，建议执行不带参数的完整安装。

### 6.1 处理 pip 网络错误

典型错误：

```text
Failed to establish a new connection: [Errno 101] Network is unreachable
Could not find a version that satisfies the requirement cerberus
```

这通常不是 `cerberus` 包不存在，而是 `pip` 无法访问 PyPI。先验证：

```bash
curl -I --connect-timeout 15 https://pypi.org/simple/cerberus/
python3 -m pip index versions cerberus
```

如果失败，重新执行第 3.4 节中的 `export` 代理命令，然后重新运行：

```bash
cd ~/PX4-Autopilot
bash Tools/setup/ubuntu.sh
```

不要使用 `sudo pip install`，否则可能破坏 Ubuntu 的系统 Python 环境。

## 7. 重启 WSL

安装脚本完成后，在 Ubuntu 中退出：

```bash
exit
```

然后在 **PowerShell** 中执行：

```powershell
wsl --shutdown
wsl -d Ubuntu-24.04
```

`wsl --shutdown` 相当于彻底重启 WSL 虚拟机，能让新安装的软件、用户组和环境配置完整生效。

## 8. 检查 WSLg 图形环境

回到 Ubuntu 终端后执行：

```bash
echo "$WAYLAND_DISPLAY"
echo "$DISPLAY"
```

正常情况下至少应看到类似：

```text
wayland-0
:0
```

如果两者都是空值，Gazebo 图形窗口可能无法显示，需要先更新 WSL。可在管理员 PowerShell 中运行：

```powershell
wsl --update
wsl --shutdown
```

## 9. 编译并运行固定翼仿真

### 9.1 标准固定翼 Cessna

```bash
cd ~/PX4-Autopilot
make px4_sitl gz_rc_cessna
```

命令解释：

- `make`：调用 PX4 构建系统；
- `px4_sitl`：构建在本机运行的软件在环飞控；
- `gz_rc_cessna`：启动 Gazebo Harmonic 中的标准固定翼 Cessna 模型。

第一次编译耗时较长，后续增量编译会更快。

### 9.2 高级气动固定翼

```bash
cd ~/PX4-Autopilot
make px4_sitl gz_advanced_plane
```

`gz_advanced_plane` 使用更高级的升阻力模型，更适合：

- 气动参数匹配；
- 固定翼姿态和航迹控制验证；
- 控制器鲁棒性测试；
- 自定义机型模型验证。

### 9.3 无图形界面运行

如果只需要自动化测试或暂时排查图形问题：

```bash
cd ~/PX4-Autopilot
HEADLESS=1 make px4_sitl gz_rc_cessna
```

## 10. 判断仿真是否启动成功

成功时通常会看到：

- PX4 编译完成；
- Gazebo 中出现固定翼模型；
- PX4 终端出现 `pxh>` 提示符；
- 没有持续出现模型、传感器或 Gazebo 连接错误。

可在 `pxh>` 控制台中检查：

```text
commander status
listener vehicle_status
listener vehicle_attitude
listener airspeed_validated
```

结束仿真通常使用：

```text
Ctrl+C
```

## 11. QGroundControl

QGroundControl 用于：

- 查看飞行器状态和地图；
- 设置 PX4 参数；
- 上传任务和固定翼起降航线；
- 查看 MAVLink Inspector；
- 下载和初步分析飞行日志。

在 WSLg 环境中可运行 Linux AppImage，也可以在 Windows 中安装 QGroundControl。若 Windows 版无法自动发现 WSL2 内的 SITL，需要进一步检查 WSL 网络模式和 UDP 通信；初次验证建议优先在 WSLg 中运行 Linux 版，使 QGroundControl 与 SITL 位于同一 Linux 网络环境。

Linux AppImage 下载完成后的运行方式：

```bash
chmod +x QGroundControl*.AppImage
./QGroundControl*.AppImage
```

## 12. 日志与控制算法验证

SITL 日志通常位于：

```text
~/PX4-Autopilot/build/px4_sitl_default/rootfs/log/
```

主要是 `.ulg` 文件，可使用以下工具分析：

- PX4 Flight Review；
- `pyulog`；
- PlotJuggler；
- 自编 Python/MATLAB 数据分析程序。

建议按算法层级选择接入方式：

| 算法类型 | 推荐方式 |
|---|---|
| 姿态、角速度、空速、TECS、NPFG 等核心闭环 | 修改或新增 PX4 内部模块 |
| 轨迹规划、路径规划、编队或上层制导 | ROS 2 / uXRCE-DDS Offboard |
| 快速外部控制和任务验证 | MAVSDK / MAVLink |
| 参数调节与基线对比 | QGroundControl + ULog |

低层高速控制回路优先放在 PX4 内部，避免 Offboard 通信时延和频率波动影响实验结论。

## 13. 版本与可重复实验

当前浅克隆的是持续更新的 `main` 分支，适合学习和试用新功能。正式进行论文、控制器对比或回归测试时，应固定到发布标签或具体提交。

记录当前提交：

```bash
cd ~/PX4-Autopilot
git rev-parse HEAD
git describe --always --tags
```

实验记录至少应保存：

- PX4 提交号；
- Gazebo 模型及其参数；
- PX4 参数文件；
- 控制算法代码版本；
- 仿真场景、风场和初始条件；
- `.ulg` 日志；
- 评价指标和绘图脚本。

## 14. 常见问题速查

### Git 克隆出现 `RPC failed` 或 `early EOF`

使用浅克隆、HTTP/1.1、单并发和代理。主仓库与子模块分开下载。

### 子模块下载中断

重复执行：

```bash
cd ~/PX4-Autopilot
git submodule update --init --recursive --depth 1 --jobs 1
```

### `pip` 提示找不到 `cerberus`

先检查 PyPI 网络。该提示通常由网络不可达引起，不代表包不存在。设置 `http_proxy` 和 `https_proxy` 后重新运行安装脚本。

### 代理显示 `Connection refused`

检查端口是否为 `7897`、代理软件是否运行、是否开启 `Allow LAN`，并分别测试 `127.0.0.1` 和 WSL 默认网关地址。

### Gazebo 没有窗口

检查 `$WAYLAND_DISPLAY` 和 `$DISPLAY`，更新并重启 WSL；也可先用 `HEADLESS=1` 验证编译和 SITL 是否正常。

### 忘记命令应该在哪里运行

- `git`、`bash Tools/setup/ubuntu.sh`、`make px4_sitl ...`：Ubuntu；
- `wsl -l -v`、`wsl --shutdown`、`wsl --update`：PowerShell。

## 15. 推荐学习顺序

1. 成功启动 `gz_rc_cessna`，理解 PX4、SITL、Gazebo 和 QGroundControl 的关系。
2. 完成手动或任务模式的固定翼起飞、航线飞行和降落。
3. 学习 ULog，绘制姿态、角速度、空速、位置和控制量。
4. 阅读固定翼姿态控制、TECS 和 NPFG 模块的数据流。
5. 建立同一场景下的基线控制器指标。
6. 接入自定义控制算法，并保存代码提交、参数和日志。
7. 在风扰、参数偏差和传感器噪声下进行鲁棒性测试。
8. 固定 PX4 版本和仿真条件，形成可重复的实验流程。

