# Linux文件操作
## 文件与目录
学习以下命令：
```bash
pwd
ls
ls -la
mkdir -p ~/linux-git-lab/data
cd ~/linux-git-lab
touch notes.txt
cp notes.txt ./data/notes-copy.txt
mv data/notes-copy.txt data/px4-notes.txt
find . -maxdepth 2 -type f
```
1. `pwd`：查看当前位置
![pwd](assets/images/Linux文件操作_image.png)
2. `ls`：列出当前目录内容(list)
![ls](assets/images/Linux文件操作_image-1.png)
3. `ls -la`：显示所有内容和详细信息，包括以`.`开头的隐藏文件，权限、所有者、大小和修改时间。
![ls -la](assets/images/Linux文件操作_image-2.png)
4. `mkdir -p`，`cd`：递归创建目录和切换当前工作目录
![mkdir&cd](assets/images/Linux文件操作_image-3.png)
`-p`保证当我们要创建的目录上级目录也不存在时，能够一同创建，如果上级目录存在，这样也不会报错。
5. `touch`创建空文件
![touch1](assets/images/Linux文件操作_image-4.png)
![touch2](assets/images/Linux文件操作_image-5.png)
**该命令会在当前目录下创建一个空文件，所以要注意当前位置**。
![pos1](assets/images/Linux文件操作_image-7.png)
![pos2](assets/images/Linux文件操作_image-6.png)
6. `cp`复制
`cp "当前目录下的文件" "目标目录下文件名"`
如上图所示，可以将文件复制到另外的路径下。
7. `mv`移动或重命名
```bash
mv notes-copy.txt new_name.txt
mv note-copy.txt ~/linux-git-lab/New_Name.txt
mv "当前路径下文件" "目标路径下文件(新文件名)"
```

![mv](assets/images/Linux文件操作_image-8.png)
8. `find`按条件查找文件
`find`查找文件或目录
`.`查找的起始位置，"."表示当前目录
`-maxdepth 2`最多向下搜索两层
`-type f`只显示普通文件，`f`表示file
![find](assets/images/Linux文件操作_image-9.png)
9.  `~`当前Linux用户的主目录
通常使用`cd ~`

## 文件权限
学习以下常用命令：
```bash
cd ~/linux-git-lab
touch test.sh
ls -l test.sh
chmod u+x test.sh
ls -l test.sh
```
运行上述命令后结果如下
![result2](assets/images/Linux文件操作_image-10.png)
`ls -l`查看文件的权限
注意到`test.sh`文件的权限从`-rw-r--r--`变为`-rwxr--r--`。
其中
- `r`读取
- `w`写入
- `x`执行
三组权限分别对应“所有者”、“用户组”和“其他用户”
`chmod u+x`给所有者增加执行权限。
`u`所有者
`g`用户组
`o`其他用户
`a` = `ugo`全体成员
添加权限用`+`，删除权限用`-`

## 进程
