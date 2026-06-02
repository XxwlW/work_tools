XxwlW: 解析该项目，分析如何对上游数据进行可视化

Workspace: 正在收集工作区信息正在筛选到最相关的信息GitHub Copilot

下面是对工程中“上游数据到可视化”路径的解析与可操作流程（简短、面向工程实践）。

关键模块与文件（均为仓内路径，可点击打开）
- 可视化工具总入口、构建配置： CMakeLists.txt
- 可视化程序主逻辑： `tte::planning::plotviewer::main` — main.cpp
- 实时订阅/反序列化入口（接收上游数据）： zmq_subscriber.cpp
- 数据记录（将接收流写入 multi-frame 文件）： `tte::planning::plotviewer::DataRecording` — data_record.cpp
- 单帧/回放逻辑： `tte::planning::plotviewer::PlayBackSingleFrameAndAnimation` — playback_single_frame.cpp
- Hybrid-A* 回放： `tte::planning::plotviewer::PlayBackHybridAStar` — main_playback_hybrid_a_star.cpp
- 可视化数据结构（protobuf）： `tte::planning::extra_plotdata::ExtraPlotData` — extra_plot_data.pb.h
- 上游发布端（将数据发给 plot_viewer）： zmq_publisher.cpp
- multi-frame 文件实现（写/读帧）： multi_frame_file.h
- 从目标设备拉取记录数据（脚本）： retrieve_data_for_playback.sh
- plot_viewer 配置： plot_viewer_config.h

总体工作流（最少步骤）
1. 编译 plot_viewer / play_back 工具  
   - 构建目标在 CMakeLists.txt。在项目根或 build 目录使用 cmake，随后构建对应 target（示例命令见下）。
2. 实时可视化（在线）  
   - 上游模块将 protobuf/序列化数据通过 ZMQ 发布，发布实现位于 zmq_publisher.cpp。  
   - 运行可视化程序（plot_viewer），其通过 zmq_subscriber.cpp 订阅并在 UI/绘图线程中显示（主逻辑在 main.cpp）。
3. 记录流以便离线回放  
   - plot_viewer 中的记录线程由 `tte::planning::plotviewer::DataRecording` 控制，会把每帧数据写入 multi-frame 文件（使用 `MultiFrameFile`，定义见 multi_frame_file.h）。保存目录/开关在 plot_viewer_config.h 中配置（字段如 kRecordDataFileDir、kEnableRecordDataToFile、kEnableRecordLogToFile）。
4. 拉取/传输记录文件（从设备）  
   - 若数据在目标板上，使用脚本 retrieve_data_for_playback.sh 将 /opt/planning/... 下的数据拉回到工作区 data 目录。
5. 离线回放（播放已记录数据）  
   - 使用 play_back 可执行文件（由 CMakeLists.txt 生成），回放逻辑在 `tte::planning::plotviewer::PlayBackSingleFrameAndAnimation` 与 `tte::planning::plotviewer::PlayBackHybridAStar` 中。play_back 可以直接读取旧版单文件（data_N）或 multi-frame 文件并做 matplotlib 绘图（依赖 python matplotlib 绑定，见 CMake 链接 python3.7m）。

关键注意点
- 数据格式：plot_viewer 期望的消息类型是工程的 protobuf（例如 `ExtraPlotData`），见 extra_plot_data.pb.h。SetPlotData 的反序列化逻辑在 `tte::planning::plotviewer::PlotViewer::SetPlotData` / plot_viewer.cpp（打开查看）；
- 依赖：绘图使用 matplotlib-cpp + Python，构建时要确保系统 Python 与 matplotlib 可用（CMake 中链接 python3.7m），否则只能记录但无法绘图；
- 兼容旧/新数据：playback 里有对旧单文件（data_N）和 multi-frame 文件的兼容判断（查看 playback_single_frame.cpp）；
- 若需要把其它上游模块数据接入：让上游发布符合 protobuf 定义的序列化数据或采用与 zmq_publisher.cpp 相同的消息格式/主题。

快速命令（在项目根或 build 目录）
```sh
# 进入项目顶层 build（或新建）
mkdir -p build && cd build
cmake .. 
# 构建 plot_viewer 与 play_back
cmake --build . --target plot_viewer -- -j4
cmake --build . --target play_back -- -j4

# 运行实时可视化（在发布端运行后启动）
./tools/plot_viewer/plot_viewer

# 离线回放（示例，具体参数看二进制帮助或 main_playback.cpp）
./tools/plot_viewer/play_back /path/to/data_folder frame_number

# 从目标板拉数据回工作区
./script_x86/tda4vl_scp/retrieve_data_for_playback.sh <start> <end>
```

下一步建议（可选、按需选择）
- 若上游不是以当前 protobuf 格式发送，添加一个适配层把上游消息转换为 `ExtraPlotData`（参考 extra_plot_data.pb.h），并让上游发布到与 zmq_publisher.cpp 相同的 ZMQ 主题/端口。
- 如需更灵活的回放脚本，可扩展 playback_single_frame.cpp 中的帧选择逻辑或写一个小脚本批量调用 play_back。
- 若 matplotlib 环境问题，先确认 CMake 中 python 链接并在系统中安装 matplotlib。

如果你希望我给出
- 一份最小化的“把任意上游 JSON->protobuf->plot_viewer 发布”的适配示例，或
- 一个批量回放脚本（遍历 multi-frame 文件并生成 PNG），

我可以基于上面列到的文件生成具体代码/脚本（会在回复中附带对应文件的路径链接）。

XxwlW: 解析plot_viewer实现的具体流程

Workspace: 正在收集工作区信息正在筛选到最相关的信息GitHub Copilot

下面给出 plot_viewer 的实现流程要点与关键代码位置（短、精确、可跳转）。

总体架构（高层流程）
1. 上游（planning 进程）把调试数据打包到一个本地 PlotData 对象，使用 Boost binary 序列化后通过 ZMQ 发送出去。关键生成/发送代码见：
   - 上游序列化并发送：`ZmqPublisher::DebugDataSerializationAndSend` (src/data_exchange/zmq_publisher.cpp)  
   - 上游构建 PlotData：`DataCollector::CollectDataSaveLocalAndSendByZMQ` 和 `DataCollector::CollectDataAndSendByZMQ`（生成 plot_data_、data_for_reinject_、extral_data_ 等字段）  
     - 文件：src/data_exchange/data_collector.cpp  
     - PlotData 定义：src/data_exchange/plot_data.h (`PlotData`)

2. plot_viewer 端接收、反序列化并绘图（两个线程：DataRecording 线程 + 主线程 UI 循环）：
   - 启动点：程序入口 main.cpp（当前打开文件）。主流程：
     - 先加载配置与车辆参数（LoadCommonConfigs / VehicleConfigHelper::Init）。
     - 启动数据记录线程：`tte::planning::plotviewer::DataRecording`（线程函数）。
     - 在主循环中通过互斥锁从 DataRecording 线程复制共享的 `plot_data` 到 `plot_viewer.plot_data_`，当 `new_data_came` 为 true 时触发绘图：[`main.cpp` 主循环](tools/plot_viewer/src/main.cpp)。

3. DataRecording 线程（接收 + 存盘 + 通知主线程）：
   - 文件/函数：`tte::planning::plotviewer::DataRecording` (tools/plot_viewer/src/data_record.cpp)
   - 步骤：
     - 使用 `ZmqSubcriber::RecData` 接收 ZMQ 字节流（字符串）。
     - 把接收到的字节流传给 `PlotViewer::SetPlotData` 做 Boost binary 反序列化（ia >> plot_data_）。
     - 将反序列化后的 `plot_viewer.plot_data_` 复制到全局 `plot_data`，置标志 `new_data_came = true`（由主线程读取）。
     - 若配置允许，则把帧数据写入 multi-frame 文件：使用 multi_frame_file.h 的 `AddFrame`（见 data_record.cpp 中 MultiFrameFile 使用）。
     - （可选）保存 log 到文件。

4. PlotViewer（绘图逻辑）：
   - 类/文件：`tte::planning::plotviewer::PlotViewer` 与实现 plot_viewer.cpp。
   - 关键接口：
     - `PlotViewer::SetPlotData` —— 从 std::stringstream 用 Boost binary_iarchive 反序列化到 `plot_data_`（PlotData）。
     - `PlotViewer::ParseExtralPlotData` —— 若 `plot_data_.extral_data_` 中有 protobuf（例如 `extra_plotdata::ExtraPlotData`），用 ParseFromArray 解析（见 extra_plot_data.pb.h）。
     - `PlotViewer::PlotAll` —— 顶层绘制控制：先尝试解析 extral plot data（若存在则走停车图序列），否则按配置绘制 XY/SL/ST/VT/VS 等 figures。
     - 动画与细节绘制：`PlotViewer::PlayAnimationSmoothedResult`、`PlotViewer::PlotParkingView`、以及大量 Plot* 函数（PlotGlobalPathXY、PlotObstaclesXY、PlotOutPutTrajectoryXY、PlotKappaFigure 等）。
   - 绘图库封装：使用 Matplot 封装类 `MatplotApi`（内部调用 third_party matplotlibcpp），接口如 `Plot2D`, `PlotShow`, `Pause` 等。

5. 回放（离线播放）：
   - 回放加载单帧数据：tools/plot_viewer/src/playback_single_frame.cpp。函数：
     - `PlayBackSingleFrameAndAnimation` / `PlayBackSingleFrame`：支持从 MultiFrameFile（优先）或旧版单文件（data_N）读取数据，然后调用 `plot_viewer.SetPlotData` 并 `PlotAll` 或 `PlayAnimationSmoothedResult`。
     - 帧存在检查：`CheckIfThisFrameExist`、查找下一个/上一个可用帧等函数也在该文件。

关键交互关系（简短）
- 上游构建 PlotData -> Boost 序列化 -> ZMQ 发出  
  (上游代码位于：src/data_exchange/data_collector.cpp 与 src/data_exchange/zmq_publisher.cpp)
- plot_viewer 的 DataRecording 通过 `ZmqSubcriber` 接收 -> `PlotViewer::SetPlotData` 反序列化到 `plot_data_` -> 复制到主线程可见的 `plot_data`，触发绘图
- 如果 `plot_data_.extral_data_` 包含 protobuf（extra_plotdata），PlotViewer 通过 protobuf API 解析并绘制停车视图（src/proto/extra_plot_data.pb.h）

主要相关文件和符号（可点开）
- 程序入口 / 主循环：tools/plot_viewer/src/main.cpp (`main`) — 当前打开文件  
- 数据记录线程：`tte::planning::plotviewer::DataRecording` — data_record.cpp  
- ZMQ 订阅实现：`tte::planning::plotviewer::ZmqSubcriber` — zmq_subscriber.cpp  
- PlotViewer 类与方法：`tte::planning::plotviewer::PlotViewer::SetPlotData`, `tte::planning::plotviewer::PlotViewer::PlotAll`, `tte::planning::plotviewer::PlotViewer::PlayAnimationSmoothedResult` — plot_viewer.cpp / plot_viewer.h  
- Matplot 封装：`MatplotApi` — matplot_api.cpp / matplot_api.h  
- Multi-frame 存取：src/common/multi_frame_file/multi_frame_file.h（DataRecording 使用）  
- 上游发送端（供比对）：src/data_exchange/zmq_publisher.cpp 与 data_collector.cpp（序列化 PlotData）  
- Protobuf 解析（extra_plotdata）：src/proto/extra_plot_data.pb.h；reinject 数据：src/proto/data_reinjection/reinjection_data.pb.h

快速定位路径（一次性列表）  
- main.cpp  
- data_record.cpp (`DataRecording`)  
- zmq_subscriber.cpp (`ZmqSubcriber::RecData`)  
- plot_viewer.cpp (`PlotViewer::SetPlotData`, `PlotAll`, `PlayAnimationSmoothedResult`, 各 Plot* 函数)  
- matplot_api.cpp（绘图封装）  
- playback_single_frame.cpp（离线回放）  
- zmq_publisher.cpp（上游发送）  
- data_collector.cpp（构造 PlotData，包括 `CollectDataForReinject`）  
- plot_data.h（PlotData 结构）  
- multi_frame_file.h（multi-frame 存储）

如何扩展 / 排错要点（简短）
- 若上游要可视化新字段：在上游填充 `plot_data_`（或将自定义 protobuf 写入 `plot_data_.extral_data_`），确保 plot_viewer 能解析（`ParseExtralPlotData`）并在 [`PlotViewer::Plot*`] 中绘制。参考：src/data_exchange/data_collector.cpp 的 `CollectParkingPlotData` 用法，以及 extra_plot_data.pb.h 的消息定义。  
- 若 plot_viewer 收不到数据：确认 ZMQ 地址/端口 与 plot_viewer_config.h 配置一致，并检查 `ZmqSubcriber::RecData` 的接收超时。  
- 若绘图失败或 matplotlib 问题：确认 CMake 链接的 Python/Matplotlib（CMakeLists.txt 指向 python3.7m），并检查 `MatplotApi` 输出。

需要我做什么下一步？（可选，选一个）
- 给出「如何把一个 JSON → protobuf(ExtraPlotData) → 发送到 plot_viewer」的最小示例代码（上游适配器）。  
- 给出批量把 multi-frame 文件导出成 PNG 的回放脚本（基于现有 playback 逻辑）。  

请选择一个，我会把对应的示例代码或脚本贴到工作区文件中，并附上修改位置链接。