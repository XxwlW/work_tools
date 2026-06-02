[TOC]



# 大纲

- Palnning 由三个组织层次：Scenario、Stage、Task

  - Task
    - Task 类
  - Stage
    - 类结构
  - Scenario
    - 类结构

- 规划调用路程

  - Planner

  - PlannerDispatcher

  - PlannerBase

  - PlanningComponent

  - Planning log 查看

    Planning 基础结构及调用

- planning **数据回灌及调试**

  - 数据回注
  - 调试

- DataTransfer

  - TransferRecdParkingRequestData

**Stage 主要任务：**

- OpenSpaceRoiDecider Task

  - GetParkingSpot
  - SetOriginAndStartPose
  - GetParkingBoundary
  - LoadObstacleInVertices
  - SetParkingSpotEndPose
  - 坐标系
- OpenSpaceScenarioDecider Task

  - SetParkingSubtypeIfNeed()
  - CalSlotDisToOpposite()
  - CalSlotEntranceToAroundObsDis()
  - CheckAndSetMinimalParallelSlot()
  - SetFlagIfParkingSpaceClear（）
  - DecideIfUseMiniGridSearch()
  - FormulateBoundaryConstraints()

- OpenSpaceTrajectoryProvider Task

  - OpenSpaceTrajectoryOptimizer
    - Plan()
  - HybridAStar
    - HybridAStar:: Plan()
    - HybridAStar:: ValidityCheck
  - GridSearch
    - GenerateDpMap()
- OpenSpaceTrajectoryPartition Task
- OpenSpaceFallbackDecider Task



**其他**

- OnLanePlanning
  - ComputeStitchingTrajectory
  - OnLanePlanning:: CheckIfLastPlanResultCollisionWithCurrentEnvironment

- OpenspaceUtils

  - DecideCurrentPtReferencedSafeDis

- VehicleConfigHelper

  - VehicleConfigHelper:: RearCenteredKinematicBicycleModelPredict

- 前置规划

  - OpenSpaceTrajectoryPartition:: PartitionTrajectory()

- 数据传输相关

  - data_preprocess.cpp

Planning log 查看

Planning 基础结构及调用

src/data_exchange/someip/planning_data_interface.h
![](planning/planning_main/images/planning_obsdian.assets/20250612-3.png)


# Palnning 由三个组织层次：Scenario、Stage、Task

针对不同 Scenario 的场景或问题设计不同算法，Stage 是在某个 Scenario 下的粗略规划步骤，每个 Stage 下执行其涉及的多个 Task

## Task

目录：./src/tasks/
![](planning/planning_main/images/planning_obsdian.assets/20250515.png)

task 目录下为不同的 task

### Task 类

**![](planning/planning_main/images/planning_obsdian.assets/20250515-4.png)**
Task 作为 Decider 的基类，成员函数只有 `Execute()`
各个决策器 Decider 继承 Task，分别实现各自的 Process()
 Decider 有：OpenSpaceFallbackDecider、OpenSpaceRoiDecider、PathAssessmentDecider...等
 `PathDecider、SpeedDecider` 直接继承 Task 实现 `Execute()`

优化器 Optimizer 继承 Task, 实现各自的 `Execute()`
 PathOptimizer
 SpeedOptimizer
 TrajecoryOptimizer
 上述优化器定义了纯虚函数 Process()在 Execute()中被调用，由子类实现具体的 Process()。



## Stage

### 类结构

![](../images/Planning基础结构及调用.assets/20250519-9.png)

目录: ./src/scenarios/stage.h  stage.cpp
![](planning/planning_main/images/planning_obsdian.assets/20250519-2.png)

主要函数
`virtual StageStatus Process(const TrajectoryPoint& planning_init_point, Frame* frame) = 0;`

每个场景对应 Stage 配置 在对应的 xxxproto.txt 文件 `stage_type`
如：`scenarios/park/valet_parking` 为例子：
 配置文件路径： modules\planning\conf\scenario\valet_parking_config.pb.txt
 ![](planning/planning_main/images/planning_obsdian.assets/20250519-1.png)
 该场景有两个 stage: `VALET_PARKING_APPROACHING_PARKING_SPOT`  `VALET_PARKING_PARKING`
 两个 stage 均继承与 Stage 类
 ![](planning/planning_main/images/planning_obsdian.assets/20250519-5.png)
 stage_parking.h 定义 stage VALET_PARKING_PARKING
 ![](planning/planning_main/images/planning_obsdian.assets/20250519-6.png)
 VALET_PARKING_APPROACHING_PARKING_SPOT 相同，各类实现各自的 `Process`
  `StageParking.Process()` 调用 `ExecuteTaskOnOpenSpace()` (`bool plan_ok = ExecuteTaskOnOpenSpace(frame);`)
  `StageApproachingParkingSpot.Process()` 调用 `ExecuteTaskOnReferenceLine()` (`bool plan_ok = ExecuteTaskOnReferenceLine(planning_init_point, frame);`)
 在对应 ExecteTasXXX 中 遍历定义的 TaskList，每个 Task 执行对应的 `Task->Execute`  
  Coed 如下：

  ```
for (auto* task : task_list_) {
	      const double start_timestamp = Clock::NowInSeconds();
	      const auto ret = task->Execute(frame, &reference_line_info);
	      const double end_timestamp = Clock::NowInSeconds();
	      const double time_diff_ms = (end_timestamp - start_timestamp) * 1000;
	      ADEBUG << "after task[" << task->Name() << "]: " << reference_line_info.PathSpeedDebugString();
	      ADEBUG << task->Name() << " time spend: " << time_diff_ms << " ms.";
	      RecordDebugInfo(&reference_line_info, task->Name(), time_diff_ms);
	      if (!ret.ok()) {
	        AERROR << "Failed to run tasks[" << task->Name() << "], Error message: " << ret.error_message();
	        break;
	      }
		 }
  ```

## Scenario

### 类结构

	![](planning_obsdian.assets/20250519-8.png)
目录：./src/scenarios/
![](planning/planning_main/images/planning_obsdian.assets/20250515-1.png)
​scenarios 文件夹中包含了多种场景，内部的每个文件夹就是一个 scenario 的定义和解决
scenarios 通过调用 Stage:: Process()来处理该 stage 所包含的 task，当该 stage 处理完成时，就切换到下一个 stage。只要当前的 stage 不是空、有意义，scenario 就是“未完成”的状态，从而可以继续执行接下来的 Stage。当前的 stage 是空，则所有的 stage 处理完成了，scenario 才处理完毕
流程图如下：
![](planning/planning_main/images/planning_obsdian.assets/20250519-7.png)



# 规划调用路程

## Planner

apollo/modules/planning/planner/planner.h 文件中，定义了 2 个类：Planner 类和 PlannerWithReferenceLine 类
 planner 类是所有规划器的基类，重要函数有 Init() 和 Plan()
 PlannerWithReferenceLine 类 也是继承自 Planner 类，重要函数有 PlanOnReferenceLine()

![](planning/planning_main/images/planning_obsdian.assets/20250616-1.png)
planning 模块有 4 种规划器如下：RTKReplayPlanner，PublicRoadPlanner，NaviPlanner，LatticePlanner。每个规划器针对不同的场景和问题
在这 4 个规划器中，都实现了继承自 Planner 类的 Plan()函数和继承自 PlannerWithReferenceLine 类的 PlanOnReferenceLine()函数。
在执行具体的规划任务时，都是在 Plan()中调用 PlanOnReferenceLine()，从而获得规划的轨迹结果。**最底层的规划方法，是在各规划器的 `PlanOnReferenceLine()`** 中实现

![](planning/planning_main/images/planning_obsdian.assets/20250519-10.png)

![](planning/planning_main/images/planning_obsdian.assets/20250519-11.png)

## PlannerDispatcher

在 apollo\modules\planning\planner\planner_dispatcher.h 文件中定义了 PlannerDispatcher 类，用来根据预先设定的配置文件，选择合适的 planner。该类中重要的函数是 DispatchPlanner()


## PlannerBase

apollo\modules\planning\planning_base.h
重要的函数：**RunOnce()和 Plan()**

PlanningBase 类用来描述规划的执行过程，这样就可以把通用的规划过程（planning）和具体的规划算法（planner）解耦，具有非常强的鲁棒性

![](planning/planning_main/images/planning_obsdian.assets/20250519-12.png)


## PlanningComponent

PlanningComponent 主要有三种规划器

OpenSpacePlanning - 主要的应用场景是自主泊车和狭窄路段的掉头（自主泊车规划器）
OnLanePlanning - 主要的应用场景是开放道路的自动驾驶（默认规划器）
NaviPlanning - （相对地图规划器）

apollo\modules\planning\planning_component.h 文件中定义了 PlanningComponent 类，该类包含一个 PlanningBase 类型的 unique_ptr 成员变量

![](planning/planning_main/images/planning_obsdian.assets/20250519-13.png)
在 PlanningComponent:: Proc()中，调用了 PlanningBase 类的 RunOnce 函数。根据继承与虚函数的多态特性，相对应的 planning 流程和 planner 会执行具体的规划任务。

![](planning/planning_main/images/planning_obsdian.assets/20250519-14.png)


# planning 数据回灌及调试

### 数据回注

1. `scenario_generator.sh` 脚本 用于场景创建
2. 配置文件 `commong_conf.xml` 中 设置了默认 目录为 `<kParkingSimulateDataFolder> scenario_generator </kParkingSimulateDataFolder>` 保持默认即可
3. 数据回注时，可以把实车数据利用 利用该工具导出回注格式
1. 首先利用 `playback.sh` 查看数据 或者 某一帧数据
2. 通过 `data_converter.sh` 导出实车数据到默认路径 `./data/simulation_data_parking/data_converter`
3. 利用 `scenario_generator` 将导出的数据改为回注数据
4. 注意：要将 `general_config.pb.txt` 配置文件中的工作模式 `working_mode` 设为 OPEN_SPACE_SIMULATION 在进行数据回注、调试等
5. 数据回注时要注意车型配置
6. 回注用 `plotviewer_local.sh` 进行可视化
7. 调试回注功能时，需要将 `launch.json` 中 `data_reinject` GDB 的参数手动修改 添加回注 `帧数`
**!!!!  数据转换时 障碍物类型要改... 为对应的 1~7**  
![](planning/planning_main/images/planning_obsdian.assets/20250725.png)

### 调试

1. 将 `general_config.pb.txt` 配置文件中的工作模式 `working_mode` 设为 OPEN_SPACE_SIMULATION 在进行数据回注
2. 直接进入 debug 模式 选取对应功能进行断点调试
3. 需要咨询接触 GDB 无法连接
4. 打开 `plotviewer_local.sh` ，直接运行 `planning.sh`

# DataTransfer

数据传输， mcu 数据到 planning 相关数据：

## TransferRecdParkingRequestData

决策模块发送的请求数据（debug 时 可修改相关数据 ，如强制请求重规划等...


# [OpenSpaceRoiDecider Task](open_space_roi_decider.md)

## GetParkingSpot

获取泊车信息

## SetOriginAndStartPose

设置起始位置

## GetParkingBoundary

获取计算 Left、Right Boundary

## LoadObstacleInVertices

添加障碍物





# OpenSpaceScenarioDecider Task

## SetParkingSubtypeIfNeed()

根据泊车类型及 起始位置和目标位置角度差、车位角度等设置泊车子模式类型

泊出模式下 这里判断是否可以脱困（`CheckParaParkOutIfStillTrapInSlot()`）[泊出时调试看一下这里]

## CalSlotDisToOpposite()

计算对向距离
计算 lefttop 和 righttop 两个车位角点到 对向 boundary Pt 每两个相邻点构成的 line （车位在左侧 对向为 right_boundary）的距离，最小值为车位到对向的距离
![](planning/planning_main/images/planning_obsdian.assets/20250528-1.png)
![](planning/planning_main/images/planning_obsdian.assets/20250528-2.png)


## CalSlotEntranceToAroundObsDis()

计算障碍物到入口两侧边界线的最小距离
![](planning/planning_main/images/planning_obsdian.assets/20250709.png)

只计算阈值范围内的障碍物到边界线的距离，范围外的不考虑
如上图：ABCD 点构成的障碍物不考虑
计算 EF 点构成的障碍物到边界线的距离



## CheckAndSetMinimalParallelSlot()

判断水平车位是否是小车位（带轮档）



## SetFlagIfParkingSpaceClear()

判断车位是否空旷

1. 垂直泊入、泊出：车位入口右侧 slot_entrance_right_dis  1.2m 内没有障碍物 && 左侧 slot_entrance_left_dis 0.5m 内没有障碍物 && 对向距离 > 6.6m 则为空旷 slot_dis_to_opposition

2. 水平车位泊入、泊出时 对向 > 4.6 其他和垂直一样

车位是否空旷影响 HybridAStar 相关因素权重 （搜索时 赋值变量 space_clear_flag）

车位空旷时影响：

1. RS 曲线启发因子　RsPathHeuristic

   曲率变化惩罚减小 规划路径太短惩罚减小

   ```C++
   curvature_variation_penalty *= 0.4;
   traj_too_short_penalty *= 0.65;
   ```



2. 逆向搜索时进入车位直线 DecideIfEableBackwardSearchPreferMoveStraight

   当车身与 EndPos 角度 > 50 或者 车身距 Endpos 较远（两个多变形重合面积 占 总面积 的比例 < 0.2）时考虑，若空旷：

   ```c++
   backward_search_least_dis_befor_rspath_ = veh_length * 0.35; 
   backward_search_prefer_move_straight_dis_ = veh_length * 0.35;
   backward_search_prefer_move_straight_enhance_first_factor_ = 1.0;
   backward_search_prefer_move_straight_enhance_second_factor_ = 0.5;
   ```



## DecideIfUseMiniGridSearch()

不同泊车模式下对不同类型车位进行判断是否使用小栅格、极小栅格进行搜索

不同栅格大小 设置不同搜索步长 setp_size 及 探索节点个数

垂直车尾泊入场景：
RQOP 为 endpos box
MNLG 为 startpos box
橙色 box 为 end_box_half（膨胀后的 `LateralExtend(0.8)`） 用于与障碍物遍历每个 Obs 的相邻两个点 构成的 line_seg 线判断 进行碰撞检测( `HasOverlap()`)  ==> 主要用于进行检查 EndPos 是否贴近障碍物
如果贴近 则 用 小栅格 mini_grid_search 进行搜索规划

![](planning/planning_main/images/planning_obsdian.assets/20250528.png)



## FormulateBoundaryConstraints  构建边界约束



### AdjuctObstaclesVerticesConsideringSafeDis


#### AdjuctObstaclesVerticesUsingAdaptingHybridSafeDis

![](planning/planning_main/images/planning_obsdian.assets/20250715.png)

#### ExpandCloseHullObstacleVerticesUsingAdaptingHybridSafeDis

封闭障碍物凸包（方柱等）进行调用 进行自适应安全距离设置



#### ShrinkMapBoundarySingleConvexSegmentUsingAdaptingHybridSafeDis

非封闭障碍物（边界点） 进行自适应安全距离设置



#### DecideCurrentPtReferencedSafeDis



## LoadObstacleInHyperPlanes



# OpenSpaceTrajectoryProvider Task

Path : `src/tasks/open_space_trajectory_generation/open_space_trajectory_provider.cpp`

1. 先判断是否是最后一次规划，若是最后一次规划 则跳过该 Task
 `frame_->parking_reuse_last_plan_` 在哪赋值？

2. 获取规划相关数据
 `frame_->planning_start_point_`
    初始值为世界坐标系下的车辆起始位置
    ![](planning/planning_main/images/planning_obsdian.assets/20250529-1.png)
    ![](planning/planning_main/images/planning_obsdian.assets/20250529-2.png)

3. 通过比较车辆与终点的角度与距离以及车速来判断车辆是否到达终点。
 `IsVehicleNearDestination`
    若到底终点 停止规划
    `GenerateStopTrajectory`
4. 调用 `OpenSpaceTrajectoryOptimizer::Plan()`  进行规划

```
	Status status = open_space_trajectory_optimizer_->Plan(
	      frame_->local_view_.routing_response->parking_request_info_,
	      stitching_trajectory, end_pose, XYbounds, rotate_angle, translate_origin,
	      obstacles_edges_num, obstacles_A, obstacles_b, obstacles_vertices_vec,
	      &time_latency, frame_);
```

## OpenSpaceTrajectoryOptimizer

### Plan()

传入参数赋值到临时局部变量
对点进行转换到车身坐标系 `OpenSpaceTrajectoryOptimizer::PathPointNormalizing` 该代码与 `OpenspaceUtils::PathPointNormalizing` 相同 (OpenSpaceTrajectoryOptimizer 中的移植到 OpensapceUtils)
之后调用 混合 AStarPlann()
`warm_start_->Plan()` ==> `HybridAStar::Plan()`

## HybridAStar

#### HybridAStar:: Plan()

1. 获取时间戳、清空 `open_list`  `close_list` `open_pq`
2. `LoadObstacles()`
 加载障碍物

3. `DetermineSearingDirection()`
 标志位：use_backward_search_ 作为后续搜索的 flag
   根据场景判断使用正向搜索还是 反向搜索
    Polygon2d(xxxbox).ComputeIoU(xxxbox2) //判断 两个 box 是否有交叉
4. `LoadStartAndEndNode()`
 加载搜索起点和终点

5. `DesignSensitiveAreas()`
 敏感区域设定：
    垂直车尾入库 敏感区域如下 （area 2 、3 需要进行障碍物检查）
    ![](planning/planning_main/images/planning_obsdian.assets/20250530.png)
6. [[#GenerateDpMap() |GenerateDpMap()]]
 生成 AStar 搜索的 DP 表

7. `CallPreHybridAStarPlan()`



#### HybridAStar:: ValidityCheck

`vehicle_box` 是 EndPos 的车位姿 box，判断车身位姿是否超出 ROI 区域
![](planning/planning_main/images/planning_obsdian.assets/20250603.png)



## GridSearch

### GenerateDpMap()

生成 HybridAStar 的代价 DP 矩阵 `dp_map_`  `open_pq、 open_set`



## OpenSpaceTrajectoryPartition Task



## OpenSpaceFallbackDecider Task



# VehicleConfigHelper

## VehicleConfigHelper:: RearCenteredKinematicBicycleModelPredict

轨迹拼接时调用 `ComputeStitchingTrajectory()`
通过向前欧拉 根据当前车辆后轴中心位置预测  predicted_time_horizon 后的车辆位置
预测时间默认：predicted_time_horizon 0.4s
采样间隔：rc_kinematic_bicycle_model_dt 默认为 0.05s



# OpenspaceUtils

## DecideCurrentPtReferencedSafeDis

车位附近的方柱安全距离设定



# OnLanePlanning

## ComputeStitchingTrajectory

![](planning/planning_main/images/planning_obsdian.assets/20250715-3.png)
![](planning/planning_main/images/planning_obsdian.assets/20250715-2.png)

```
if (是停车场景): 
    1. 判断是否请求重新规划 && 是否接受请求 ==> CheckIfReplanRequestAcceptable()
    2. 如果需要重新规划：
        - 设置 frame_->parking_reuse_last_plan_ = false
        - 设置 ReplanType
    3. 否则：
        - 检查上一帧轨迹最后一步是否与障碍物碰撞 ==> CheckIfLastPlanResultCollisionWithCurrentEnvironment()
        - 如果没有碰撞：
            - 是否能复用旧轨迹？==> QueryStitchTrajectory()
	            - 能：复用
	            - 不能：重新初始化
        - 如果有碰撞：设置为自重规划 ==> need_reinit_self_replan = true; frame_->replan_type_ = ReplanType::SELF_COLLISION;
        - 如果触发自重规划，再判断是否要抑制这次自重规划 ==> CheckIfSuppressThisSelfReplan()
    4. 判断是否 gear 换挡引发的 simulation 重规划 ==> 仿真模式下：OPEN_SPACE_SIMULATION
    5. 如果最终上一帧轨迹无法复用，进行重新规划 ==> ReinitStitchTrajectory()
else:
    - 使用检测是否能服用上一帧轨迹？ 或 fallback 到 Reinit
```

！！ 重规划相关：
在车位外 只有车辆静止时决策才会请求 静态重规划；
进入车位 决策可以请求动态重规划

## OnLanePlanning:: CheckIfLastPlanResultCollisionWithCurrentEnvironment



# 前置规划

前置规划（Pre-Planning） 是自动驾驶系统中在正式路径规划（如 Hybrid A*）前进行的一种轻量级、粗粒度路径预探测过程。其核心目的是判断当前环境下是否有可能到达目标位姿、是否存在合理通路，从而：
提供启发式信息（如 A* Cost Map）
估算可达性与路径方向（D 档 / R 档）
降低主规划搜索空间，提高稳定性与成功率
🚗 前置规划核心流程简介（基于 Apollo）
以 Apollo 中 PreHybridAStar 及 GridSearch:: GenerateDpMap() 为例，其典型步骤如下：

1. 构造代价地图（DpMap）
   使用栅格 A *（Grid A*）方法从目标点反向构建所有可通达栅格的 cost map（反向 DP 图）：
   grid_search.GenerateDpMap(ex, ey, XYbounds_, obstacle_linesegments);
   每个栅格记录从目标点出发的最小代价，可用于主规划的启发函数（heuristic）。

2. 搜索简化轨迹（PreHybridAStar:: Plan）
   在 DP 图中，利用简化的状态空间搜索一条从起点到终点的可行粗路径：
   状态包括位置 (x, y)、航向角 φ
   约束考虑转向半径、车宽、障碍物碰撞等
   若启发函数质量足够高，则可找到低成本方向解（前进/倒车）

3. 结果分析与输出调整
   得出轨迹方向（D 档或 R 档）
   提取末端状态（轨迹终点方向、steer）
   决定主规划器起点或终点状态（根据是否逆向搜索）



#### OpenSpaceTrajectoryPartition:: PartitionTrajectory()

分割路径

``` 判断档位
heading_angle = trajectory_point.path_point().theta();
const Vec2d tracking_vector(next_trajectory_point.path_point().x() -
								trajectory_point.path_point().x(),
							next_trajectory_point.path_point().y() -
								trajectory_point.path_point().y());
tracking_angle = tracking_vector.Angle();
```



# 数据传输相关

### data_preprocess.cpp

 回注数据时的 相关数据转换入口：`ReverseSimulateDataTransferForReinject`
![](planning/planning_main/images/planning_obsdian.assets/20250616.png)

