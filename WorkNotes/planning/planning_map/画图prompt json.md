```

你是一名自动泊车算法可视化助手。

请分析我提供的C/C++场景构造代码。

任务：

第一步：识别场景元素

识别以下对象：

* Main Boundary
* Sub Boundary
* Parking Slot
* Obj1（后车）
* Obj2（前车）
* Start Pose
* Goal Pose
* Anchor Point
* Coordinate System

第二步：提取几何参数

输出：

{
"slot_side": "right|left",
"anchor":
{
"x": ...,
"y": ...,
"yaw": ...
},

```
"obj1":
{
    "x": ...,
    "y": ...,
    "yaw": ...
},

"obj2":
{
    "x": ...,
    "y": ...,
    "yaw": ...
},

"main_boundary":
[
    [x0,y0],
    [x1,y1],
    ...
    [x5,y5]
],

"sub_boundary":
[
    [x0,y0],
    [x1,y1]
]
```

}

第三步：生成场景图

绘制要求：

* 正交俯视图
* XY坐标系
* 主边界黑色粗线
* 子边界黑色粗线
* 停车位黑色矩形
* 起始车辆蓝色
* 目标车辆绿色
* Obj1红色
* Obj2橙色
* 显示关键坐标
* 保持真实比例
* 工程CAD风格

第四步：

生成可直接运行的Python Matplotlib代码。


```