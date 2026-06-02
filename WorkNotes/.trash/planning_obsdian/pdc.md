数据jie'g

```
typedef struct {
  uint8_INF id;
  uint8_INF type;
  uint8_INF polygon_point_num;
  size_t  flag;  
  Point2D_INF polygon_point_list[OBSTACLE_POLYGON_POINT_MAX_NUM];
} OppositieSlot_INF;
```

```

```



现在src/data_exchange/someip/planning_data_interface.h新增一个数据结构存放对向检测到的车位信息，数据结构如下：
typedef struct {
  uint8_INF id;
  uint8_INF type;
  uint8_INF polygon_point_num;
  size_t  flag;  
  Point2D_INF polygon_point_list[OBSTACLE_POLYGON_POINT_MAX_NUM];
} OppositieSlot_INF;

typedef struct{
  Time_Stamp_t_INF timestamp_ms;
  uint8_INF oppo_slot_num;
  OppositieSlot_INF oppo_slot_list[OBSTACLE_MAX_NUM];
} OppositieSlotInfo_INF;

在src/data_exchange/someip/planning_input_sub.h 中类

class InputDataSub {
 public:
  Localization_INF localization_;
  Chassis_INF chassis_;
  GlobalPath_INF global_path_;
  BoundaryMap_INF map_boundary_;
  ObstaclesInfo_INF obj_info_;
  ParkingRequest_INF parking_request_;
  OppositieSlotInfo_INF oppo_slot_info_;
}; 并在对应CPP中定义新增：

void SubInputDataInit(InputDataSub * input_data) {
  TLOG_INFO << "SubInputDataInit...";
  memset(input_data, 0, sizeof(InputDataSub));

  input_data->chassis_.timestamp_ms = 0;
  input_data->localization_.timestamp_ms = 0;
  input_data->global_path_.timestamp_ms = 0;
  input_data->global_path_.PtCnt = 0;
  input_data->map_boundary_.timestamp_ms = 0;
  input_data->obj_info_.timestamp_ms = 0;
  input_data->parking_request_.timestamp_ms = 0;
  input_data->oppo_slot_info_.timestamp_ms = 0;
}

如何修改代码可以根据传入的对向车位信息进行绘图和回放显示？









XxwlW: 解析整个项目，梳理出src/data_exchange/someip/planning_data_interface.h ObstaclesInfo_INF 是如何转为tools/plot_viewer/src/plot_viewer.cpp 中使用的plot_data_.static_obstacles_数据的

Workspace: 正在收集工作区信息正在筛选到最相关的信息下面按流水线把数据流走向做成图谱并给出关键代码位置（每一项都附上符号/文件链接），便于逐步跟踪从 SOMEIP 消息到 plot_viewer 的静态障碍物显示。

总体流程（简短版）
1) SOMEIP 消息结构 `tte::planning::ObstaclesInfo_INF` 被放入订阅端包装结构 `tte::planning::InputDataSub`。处理/初始化在 `tte::planning::SubInputDataInit` 中。
   - 文件：src/data_exchange/someip/planning_data_interface.h
   - 文件/符号：`tte::planning::InputDataSub` / `tte::planning::SubInputDataInit`

2) InputDataSub（包含 `obj_info_` 即 `ObstaclesInfo_INF`）由输入层转发到规划模块（data_preprocess / data_transfer / PlanningComponent）。这一步将 SOMEIP 原始结构解包并转换为内部运行时的障碍物集合（通常转为 `common::Obstacle` / planning 内部的 map/list）。
   - 入口示例：数据预处理/传输模块 data_preprocess.cpp
   - 规划组件：`planning_component.h`（pipeline 中负责维护运行时数据结构，供 DataCollector 读取）

3) 在规划运行时，内部障碍物以 map<string, Obstacle>（变量名示例为 `obstacles_`）的形式存在于规划上下文中。`DataCollector` 通过链接到该上下文读取障碍物集合（`SetDataLink` / `SetDataLink(const PlanningComponent&)`）。
   - 读入点：`tte::planning::DataCollector::SetDataLink`
   - 文件：src/planning_component.h
   - 文件：src/data_exchange/data_collector.cpp

4) `DataCollector::CollectStaticObstacleData()` 把运行时的障碍物转换为用于可视化的轻量结构并填充到 `plot_data_.static_obstacles_` 中。关键行为：
   - 遍历 `obstacles_`（规划内部障碍集合）
   - 对每个障碍取感知多边形顶点（PerceptionPolygon / GetAllVertices）
   - 将顶点坐标填入 `StaticObstacle`（plot 数据结构），最后 push 到 `plot_data_.static_obstacles_`
   - 相关实现位于：[`src/data_exchange/data_collector.cpp` 中的 CollectStaticObstacleData()](src/data_exchange/data_collector.cpp)
   - plot 数据结构定义：src/data_exchange/plot_data.h，其中包含 [`tte::planning::StaticObstacle`] 与 `PlotData::static_obstacles_`

   参考代码片段位置：
   - 转换逻辑：`tte::planning::DataCollector::CollectStaticObstacleData`
   - plot 结构：plot_data.h（查看 `StaticObstacle` 与 `PlotData` 成员）

5) `DataCollector` 把填好的 `plot_data_` 序列化并通过 ZMQ 或本地文件发送给 plot viewer（函数例：`CollectDataSaveLocalAndSendByZMQ` / 发送入口）。  
   - 发送/保存：[`src/data_exchange/data_collector.cpp` 中 CollectDataSaveLocalAndSendByZMQ 等函数](src/data_exchange/data_collector.cpp)

6) plot_viewer 端接收并反序列化该 plot 数据：在 `tte::planning::plotviewer::PlotViewer::SetPlotData` 中用 boost 序列化（binary_iarchive）恢复为 `plot_viewer.plot_data_`（含 `static_obstacles_`）。
   - 反序列化：[`tools/plot_viewer/src/plot_viewer.cpp` 中的 PlotViewer::SetPlotData](tools/plot_viewer/src/plot_viewer.cpp)
   - plot_viewer 类定义：plot_viewer.h

7) 最后可视化环节：`tte::planning::plotviewer::PlotViewer::PlotObstaclesXY` 读取 `plot_data_.static_obstacles_` 并调用绘图接口（MatplotApi）把每个障碍多边形画出，若 `obs.obs_id` 是数字则在中心位置用 MatplotApi::Text 标注 ID。
   - 绘制实现：[`tools/plot_viewer/src/plot_viewer.cpp` 中 PlotViewer::PlotObstaclesXY](tools/plot_viewer/src/plot_viewer.cpp)

关键代码/符号汇总（点击可以打开相应文件）
- SOMEIP 类型定义：`tte::planning::ObstaclesInfo_INF` — planning_data_interface.h
- InputDataSub / 初始化：`tte::planning::InputDataSub` / `tte::planning::SubInputDataInit` — planning_input_sub.h, planning_input_sub.cpp
- 规划/数据管线入口：src/data_exchange/data_preprocess.cpp
- 运行时障碍物容器 & PlanningComponent：src/planning_component.h
- DataCollector（转换到 plot_data）：`tte::planning::DataCollector::CollectStaticObstacleData` — data_collector.cpp
- plot 数据结构（StaticObstacle / PlotData::static_obstacles_）：src/data_exchange/plot_data.h
- plot_viewer 反序列化：`tte::planning::plotviewer::PlotViewer::SetPlotData` — plot_viewer.cpp
- plot_viewer 绘制：`tte::planning::plotviewer::PlotViewer::PlotObstaclesXY` — plot_viewer.cpp
- plot_viewer 类声明：tools/plot_viewer/src/plot_viewer.h

简短结论
- 核心转换链路是：
  `ObstaclesInfo_INF` (SOMEIP 消息)
   -> 包装到 `InputDataSub::obj_info_`
   -> 由 data_preprocess / planning_component 解包转为运行时 `Obstacle` 集合（obstacles_）
   -> `DataCollector::CollectStaticObstacleData` 填充 `plot_data_.static_obstacles_`
   -> 序列化 / 发送 -> `PlotViewer::SetPlotData` 反序列化
   -> `PlotViewer::PlotObstaclesXY` 绘制

如果需要，我可以：
- 给出 `CollectStaticObstacleData()` 到 `PlotObstaclesXY()` 的关键代码行级摘录并标注变量流向；
- 或者列出从 `ObstaclesInfo_INF` 字段映射到 `StaticObstacle` 字段的逐字段对应关系（需要我提取对应结构定义并比对）。

XxwlW: 现在要在src/data_exchange/someip/planning_data_interface.h新增一个数据结构存放对向检测到的车位信息，数据结构如下：
typedef struct {
  uint8_INF id;
  uint8_INF type;
  uint8_INF polygon_point_num;
  size_t  flag;  
  Point2D_INF polygon_point_list[OBSTACLE_POLYGON_POINT_MAX_NUM];
} OppositieSlot_INF;
如何修改代码可以根据传入的对向车位信息进行绘图和回放显示？


GitHub Copilot: GitHub Copilot

下面给出最小可行的改动方案与示例代码片段，说明如何把新的 SOMEIP 结构 OppositieSlot_INF（对向车位）接入到可视化回放链路：SOMEIP -> 运行时容器 -> DataCollector 填充 PlotData -> PlotViewer 反序列化并绘制。每段代码为可直接补入对应文件的补丁样例，必要时需根据实际字段名（运行时容器位置）做小调整。

要点步骤（简短）
1. 在 planning_data_interface.h 添加 OppositieSlot_INF 定义并确保 SOMEIP 解析代码把该数组/列表填入 InputDataSub（或等价运行时容器）。
2. 在 plot_data.h 中添加 Plot 用的轻量结构 OppositeSlot 并加入 PlotData::opposite_slots_，并为其添加 boost 序列化支持（回放需要）。
3. 在 src/data_exchange/data_collector.* 中新增 CollectOppositeSlotData()，把运行时的 OppositieSlot_INF 转为 PlotData::OppositeSlot 并加入 plot_data_（序列化前）。
4. 在 plot_viewer.cpp 中新增绘制函数 PlotOppositeSlotsXY()，并在主绘制流程中调用，显示多边形与 id。

代码补丁示例

1) 新增 SOMEIP 结构（src/data_exchange/someip/planning_data_interface.h）
````c
// ...existing code...
typedef struct {
  uint8_INF id;
  uint8_INF type;
  uint8_INF polygon_point_num;
  size_t  flag;
  Point2D_INF polygon_point_list[OBSTACLE_POLYGON_POINT_MAX_NUM];
} OppositieSlot_INF;
// ...existing code...
````
（提示：如果 InputDataSub/解析器里没有保存对向车位数组，需在 src/data_exchange/someip/planning_input_sub.h/.cpp 中添加成员，例如 vector<OppositieSlot_INF> opposite_slots_; 并在 SOMEIP 回调中填充。）

2) 在 plot 数据结构里加入轻量类型并支持序列化（src/data_exchange/plot_data.h）
````c++
// ...existing code...
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

struct OppositeSlot {
  uint32_t id;
  uint8_t  type;
  size_t   flag;
  std::vector<Point2D> polygon; // Point2D 是已有 plot 数据点类型
  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/) {
    ar & id;
    ar & type;
    ar & flag;
    ar & polygon;
  }
};

struct PlotData {
  // ...existing members...
  std::vector<OppositeSlot> opposite_slots_;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/) {
    // ...serialize existing members...
    ar & opposite_slots_;
  }
};
// ...existing code...
````
（注意：Point2D 应与已有 plot_data 中点类型保持一致；如果名不符请替换。）

3) DataCollector 中把运行时对向车位填入 plot_data_（src/data_exchange/data_collector.h / .cpp）
- 在头文件声明新函数（data_collector.h）：
````c++
// ...existing code...
class DataCollector {
 public:
  // ...existing declarations...
  void CollectOppositeSlotData();
  // ...existing code...
};
// ...existing code...
````
- 在实现文件增加转换（data_collector.cpp）。此处示例从假定运行时容器 planning_component_.opposite_slots_ 读取（也可改为从 InputDataSub、obj_info_ 等处读取）：
````c++
// ...existing code...
void DataCollector::CollectOppositeSlotData() {
  plot_data_.opposite_slots_.clear();
  // 假定存在运行时容器 planning_component_.opposite_slots_，类型为 vector<OppositieSlot_INF>
  for (const auto &src_slot : planning_component_.opposite_slots_) {
    OppositeSlot dst;
    dst.id = static_cast<uint32_t>(src_slot.id);
    dst.type = static_cast<uint8_t>(src_slot.type);
    dst.flag = src_slot.flag;
    dst.polygon.clear();
    size_t pts = std::min<size_t>(src_slot.polygon_point_num, OBSTACLE_POLYGON_POINT_MAX_NUM);
    for (size_t i = 0; i < pts; ++i) {
      Point2D p;
      p.x = static_cast<double>(src_slot.polygon_point_list[i].x);
      p.y = static_cast<double>(src_slot.polygon_point_list[i].y);
      dst.polygon.push_back(p);
    }
    plot_data_.opposite_slots_.push_back(std::move(dst));
  }
}
// 在原有收集/序列化流程中调用 CollectOppositeSlotData()
// 例如在 CollectDataSaveLocalAndSendByZMQ() 或 CollectAllData() 中加入 CollectOppositeSlotData();
````
（如果运行时容器命名不同，请改为从实际存放 OppositieSlot_INF 的结构读取。）

4) plot_viewer 中绘制对向车位（tools/plot_viewer/src/plot_viewer.cpp）
- 新增绘制函数并在合适位置调用（例如与静态障碍绘制同处）：
````c++
// ...existing code...
void PlotViewer::PlotOppositeSlotsXY() {
  for (const auto &slot : plot_data_.opposite_slots_) {
    std::vector<double> xs, ys;
    for (const auto &p : slot.polygon) {
      xs.push_back(p.x);
      ys.push_back(p.y);
    }
    if (xs.empty()) continue;
    // 闭合多边形
    xs.push_back(xs.front());
    ys.push_back(ys.front());
    // 使用项目已有绘图 API（示例为 MatplotApi）
    MatplotApi::Plot(xs, ys).set_color("magenta");
    // 标注 id 在质心处（简单质心）
    double cx = 0, cy = 0;
    for (size_t i = 0; i < xs.size()-1; ++i) { cx += xs[i]; cy += ys[i]; }
    cx /= (xs.size()-1); cy /= (xs.size()-1);
    MatplotApi::Text(std::to_string(slot.id), cx, cy).set_color("magenta");
  }
}
// 在绘制主流程（如 PlotObstaclesXY 或 DrawFrame）中加入调用：
void PlotViewer::PlotObstaclesXY() {
  // ...existing obstacle drawing...
  PlotOppositeSlotsXY();
}
// ...existing code...
````
（将 MatplotApi 调用替换为项目中实际的绘图库接口；颜色、样式可调整。）

小结与注意事项（必读）
- 必须在 SOMEIP 解包/回调代码中把 OppositieSlot_INF 填入某个运行时容器（例如 planning_component_ 或 InputDataSub），DataCollector 从该容器读取并填充 plot_data_.opposite_slots_。如果缺少这一步，PlotViewer 无法得到数据。
- PlotData::OppositeSlot 需被 boost Serialization 处理（上面示例已给出），否则回放/网络反序列化会失败。
- 修改点主要在：
  - src/data_exchange/someip/planning_data_interface.h（新增结构）
  - src/data_exchange/someip/planning_input_sub.{h,cpp}（填充运行时容器）
  - src/data_exchange/plot_data.h（plot 数据结构 & 序列化）
  - src/data_exchange/data_collector.{h,cpp}（收集并填充 plot_data_.opposite_slots_）
  - tools/plot_viewer/src/plot_viewer.{h,cpp}（反序列化后绘制）
- 调试建议：先把 DataCollector 填充后的 plot_data_ 存到本地文件并用 plot_viewer 直接打开，以便验证序列化/字段映射正确后再启用 ZMQ 实时流。

需要我给出：
- 针对你的仓库中实际运行时容器名（例如 planning_component_.xxxxx）做精确补丁？（我可以打开/查看 /media/disk/2818 下相关文件并生成精确修改补丁）
