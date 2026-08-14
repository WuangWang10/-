# 用Python读取csv文件并绘制图像
## 要调用的Python库
通过pandas库进行csv文件的读取；通过matplotlib库进行绘图
```python
import pandas as pd
import matplotlib.pyplot as plt
```

## csv文件的读取
对于指定的csv文件路径`FILE_PATH`，通过`pandas`库中的`pd.read_csv()`命令来读取csv文件中的数据。
```python
FILE_PATH = "文件绝对路径"
FILE_DATD = pandas.read_csv(FILE_PATH)
```
### 文档数据的查看
- 查看文件有几行几列：`print(FILE_DATA.shape)`
- 查看文件的所有列名（表头），并转为列表：`print(FILE_DATA.columns.tolist())`
- 查看文件的前5行数据（head括号内可指定行数）：`print(FILE_DATA.head())`
- 查看后5行：`print(FILE_DATA.tail())`
- 数据详细信息（类型，空值）：`print(FILE_DATA.info())`
- 数值列统计（均值，最值，标准差）：`print(FILE_DATA.describe())`
- 获取某一列全部数据：
    `xdata = FILE_DATA["时间"]`，`ydata = FILE_DATA["数值"]`

## 根据文件中数据绘图
1. 取出x轴、y轴数据
2. 创建画布
3. 选择绘图类型
4. 设置标签、标题、网格、图例
5. 展示/保存图片

```python
# 1.提取两列数据
x = FILE_DATA["x列名"]
y = FILE_DATA["y列名"]

# 2.创建画布，figsize(宽,高) 单位英寸
plt.figure(figsize=(10, 5))

# 3.绘图，下面分多种图形示例
plt.plot(x, y)

# 4.图表修饰
plt.title("CSV数据曲线图")   # 图表标题
plt.xlabel("横轴名称")        # X轴标签
plt.ylabel("纵轴名称")        # Y轴标签
plt.grid(True)                 # 开启网格
plt.legend(["曲线1"])          # 图例

# 5.保存图片（放在show前面，否则空白）
plt.savefig("result.png", dpi=300)

# 6.弹出图像窗口
plt.show()
```