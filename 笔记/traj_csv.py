import pandas as pd
import matplotlib.pyplot as plt

file_name = "E:/matcode/Fixed-wing-UAV/fixuav/results/aerobatic/dual_loop_cfc_optimization_summary.csv"
df = pd.read_csv(file_name)

# 查看CVS文件中有多少行多少列
print("数据形状：", df.shape)

# 查看所有列名
print("\n所有列名：")
print(df.columns.tolist())

# 查看前几行数据
print("\n数据预览")
print(df.head())

# 绘图
bspline_data = df[df["Planner"] == "bspline"]
quintic_data = df[df["Planner"] == "quintic"]

x = ["loop", "immelmann_turn"]
y_bspline = bspline_data["RMSPositionError_m"].values
y_quintic = quintic_data["RMSPositionError_m"].values

plt.plot(x, y_bspline, marker='o', label='bspline', color='#1f77b4', linewidth=2)
plt.plot(x, y_quintic, marker='o', label='bspline', color='#ff7f0e', linewidth=2)

# 解决中文显示问题（可选，如果你标题用中文）
plt.rcParams["font.sans-serif"] = ["SimHei"]
plt.rcParams["axes.unicode_minus"] = False

plt.xlabel("Maneuver（机动类型）")
plt.ylabel("RMS Position Err(m)")
plt.legend()
plt.grid(True, alpha=0.3)
plt.show()
