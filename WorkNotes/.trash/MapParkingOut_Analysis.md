# MapParkingOut.cpp 代码分析与重构指南

> 生成于 2026-05-11，基于 `libttePlanning.so` 符号表逆向分析
> 项目：AVP 自动泊车规划系统，TDA4 平台，C++14

---

## 一、文件概况

- **文件名**：`src/pnc_map/MapParkingOut.cpp`（565KB）
- **头文件**：`src/pnc_map/MapParkingOut.h`（11KB）
- **格式**：二进制编码格式（`TSD-Header` / `TSZ`），非文本源码
- **编译产物**：`bin_x86/libttePlanning.so`（not stripped，符号完整）
- **编译目标**：`tMap_` 前缀表示该函数属于地图图层（pnc_map 模块）

---

## 二、核心职责

`MapParkingOut.cpp` 是 **泊出场景的地图构建与融合模块**，负责：

| 职责 | 说明 |
|------|------|
| 车位信息计算 | 基于 VPL、超声(USS)、全景(SDG/FSD) 等传感器数据计算车位参数 |
| 地图边界融合 | 融合多种传感器数据，构建泊出所需的电子围栏和边界地图 |
| 车位构建与排序 | 根据传感器数据重建、排序、筛选可用车位 |
| 场景模式判断 | 判断当前泊出场景模式（垂直/平行/斜向） |
| 数据总线接口 | 从传感器获取输入数据，输出处理后的地图信息 |

---

## 三、函数结构总览（60+ 个函数）

### 3.1 顶层入口函数（Task 级别）

```
APAMap_ParkingOutTask()              // --- 主任务入口，调用链顶端
APAMap_ParkingOutCalMapInfo()        // 计算地图信息的核心入口
APAMap_ParkingOutUpDataMapInfo()     // 更新地图信息的核心入口
APAMap_ParkingOutCalSlotInfo()       // 计算车位信息的核心入口
```

### 3.2 初始化与调试

```
APAMap_ParkingOutDebugInit()          // 调试数据初始化
APAMap_ParkingOutBkDataBfSDGFusInit() // 备份数据初始化（SDG融合前）
APAMap_ParkingOutBkSDGOutPutDataInit()// SDG输出数据备份初始化
```

### 3.3 传感器数据获取（Input 层）

```
APAMap_ParkingOutGetSDGInfoPt()           // 获取SDG(全景)信息点
APAMap_ParkingOutGetPDCInfoPt()           // 获取PDC(超声波)信息点
APAMap_ParkingOutGetSDGInfoByParkMode()   // 按泊出模式获取SDG信息
APAMap_ParkingOutGetPDCInfoByParkSide()   // 按泊出侧获取PDC信息
APAMap_ParkingOutGetBkSDGOutPutData()     // 获取备份的SDG输出数据
APAMap_ParkingOutGetEightParkOutMode()    // 获取八种泊出模式
```

### 3.4 传感器数据筛选排序（Processing 层）

```
APAMap_ParkingOutSiftAndSeqSDGPts()           // 筛选排序SDG点
APAMap_ParkingOutSiftAndSeqPDCPts()           // 筛选排序PDC点
APAMap_ParkingOutReOrderVPLSlotPtsByParkOutMode() // 按泊出模式重排VPL车位点
```

### 3.5 车位信息计算（Slot 层）

```
APAMap_ParkingOutCalSlotInfo()                              // 计算车位信息（主入口）
APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap()      // 从全局地图计算车位参数
APAMap_ParkingOutGetSlotInfoFromVPLSlotPts()                // 从VPL车位点提取信息
APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()             // 计算车位边界点
APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()         // 基于泊出车位信息计算边界
APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()         // 基于FSD+OD地图计算边界
APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo()         // 基于OD地图的偏移计算
APAMap_ParkingOutCalSlotSlotAlignInfo()                     // 计算车位对齐信息
APAMap_ParkingOutSideSlotInfo()                             // 侧边车位信息
```

### 3.6 车位构建（Build 层）

```
APAMap_ParkingOutBuildSlotByTwoNearbySlot()                 // 由两个邻近车位构建
APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot()        // 当前车位-双邻近构建
APAMap_ParkingOutBuildSlotByOneSideNearbySlot()             // 由单侧邻近车位构建
APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot()    // 当前车位-单侧构建
APAMap_ParkingOutDeleteMainSlotBord()                       // 删除主车位边界
```

### 3.7 边界融合（Fusion 层）

```
APAMap_ParkingOutFusBoundaryBySDGInfo()       // SDG融合边界
APAMap_ParkingOutFusBoundaryByPDCInfo()       // PDC融合边界
APAMap_ParkingOutFusBoundaryByFSDMapInfo()    // FSD地图融合边界
APAMap_ParkingOutFusBoundaryByLaneLineMapInfo() // 车道线融合
APAMap_ParkingOutFusBoundaryByRefercLineMapInfo() // 参考线融合
APAMap_ParkingOutFusBoundaryByParkOutInfo()   // 综合泊出信息融合
```

### 3.8 场景与模式判断（Scenario 层）

```
APAMap_ParkingOutMapScenarioModeCheck()       // 地图场景模式检查（8个uint8_t参数）
APAMap_ParkingOutCheckIfCarPosIsValid()        // 检查车辆位置有效性
APAMap_ParkingOutCheckIfFusBoundaryPDCInfo()  // 检查PDC边界融合结果
APAMap_ParkingOutCheckIfFusBoundarySDGInfo()  // 检查SDG边界融合结果
APAMap_ParkingOutCheckIfTargetSlotIsLadderSlot() // 检查目标是否为"梯子车位"
APAMap_ParkingOutCarPosInvadeSlotBorderInfo() // 检查车辆是否侵入车位边界
```

### 3.9 终车位置处理（End Position 层）

```
APAMap_ParkingOutSetEndCarPosInOldCorSys()   // 旧坐标系下设置终车位置
APAMap_ParkingOutCenterEndCarPosInfo()       // 居中终车位置
APAMap_ParkingOutEndCarPosUpdata()           // 更新终车位置
APAMap_ParkingOutBoundarySeizeEndCarPosInfo()// 边界捕获终车位置
```

### 3.10 数据保持与回滚（Backup 层）

```
APAMap_ParkingOutSaveBkDataBfPDCFus()        // PDC融合前保存备份
APAMap_ParkingOutSaveBkDataBfSDGFus()        // SDG融合前保存备份
APAMap_ParkingOutSaveBkSDGOutPutData()       // 保存SDG输出数据
APAMap_ParkingOutSetMainSlotBordInfoByBkDataBfSDGFus() // 从备份恢复主车位边界
APAMap_ParkingOutSetSlotBordInfoByBkDataBfPDCFus()     // 从PDC备份恢复
```

### 3.11 辅助计算（Utility 层）

```
APAMap_ParkingOutLineParABCByMainSlotBord()   // 主车位边界的ABC线参数
APAMap_ParkingOutLineParABCbyPoints()          // 由点计算ABC线参数
APAMap_ParkingOutElectrFenceMapBulid()         // 电子围栏地图构建（12个float参数）
APAMap_ParkingOutPickDispersedObstacles()      // 拾取离散障碍物
APAMap_ParkingOutObliqueRowStairsInfo()        // 斜列车位台阶信息
APAMap_ParkingOutCalMapSlotPar()               // 计算地图车位参数
AlgCom_SetParkingOutObjAng()                   // 设置泊出目标角度
APAMAP_ParkingOutGetSlotBdPtByODObjs()         // 从OD障碍物获取车位边界点
```

---

## 四、数据流（Pipeline）

```
传感器数据（输入）
  ├── VPL（视觉车位线） -> APAMap_ParkingOutGetSlotInfoFromVPLSlotPts
  ├── SDG（全景影像）  -> APAMap_ParkingOutSiftAndSeqSDGPts
  ├── PDC（超声波雷达） -> APAMap_ParkingOutSiftAndSeqPDCPts
  ├── FSD（前视/环视） -> APAMap_ParkingOutFusBoundaryByFSDMapInfo
  ├── OD（障碍物检测） -> APAMap_ParkingOutPickDispersedObstacles
  └── LaneLine（车道线）-> APAMap_ParkingOutFusBoundaryByLaneLineMapInfo
        │
        ▼
    场景模式判断
  APAMap_ParkingOutMapScenarioModeCheck
  APAMap_ParkingOutCheckIfCarPosIsValid
        │
        ▼
    车位构建
  APAMap_ParkingOutBuildSlotByTwoNearbySlot   （优先）
  APAMap_ParkingOutBuildSlotByOneSideNearbySlot（降级）
        │
        ▼
    边界融合
  APAMap_ParkingOutFusBoundaryByPDCInfo
  APAMap_ParkingOutFusBoundaryBySDGInfo
  APAMap_ParkingOutFusBoundaryByFSDMapInfo
  APAMap_ParkingOutFusBoundaryByLaneLineMapInfo
  APAMap_ParkingOutFusBoundaryByRefercLineMapInfo
        │
        ▼
    终车位置处理
  APAMap_ParkingOutCenterEndCarPosInfo
  APAMap_ParkingOutBoundarySeizeEndCarPosInfo
  APAMap_ParkingOutEndCarPosUpdata
        │
        ▼
    输出地图信息
  APAMap_ParkingOutCalMapSlotPar
  APAMap_ParkingOutUpDataMapInfo
```

---

## 五、关键数据结构（从符号表推断）

| 类型 | 说明 |
|------|------|
| `APACoordinateDataCalFloatType` | 坐标点数据（float 数组/结构） |
| `APACarCoordinateDataCalFloatType` | 车辆坐标系下的坐标数据 |
| `APALineParameterKBType` | 线参数 K（斜率）和 B（截距） |
| `APALineParameterABCType` | 线参数 Ax + By + C = 0 形式 |
| `tMap_*` 系列 | 地图模块全局数据结构 |
| `ObstaclesInfo_INF` | 障碍物信息接口结构 |
| `ParkingOutRelaxEnd`（protobuf） | 泊出松弛终点（Protobuf 消息） |

---

## 六、重构建议

### 6.1 当前代码的问题

1. **C 风格函数式编程**：所有函数都是顶层 C 风格函数（`APAMap_ParkingOutXXX`），全局变量跨函数共享，没有明确的类封装
2. **函数过长**：部分函数有 `.cold` 冷分支，说明函数体非常大，编译器做了冷热代码分离优化
3. **参数过多**：如 `ElectrFenceMapBulid` 有 12 个 float 参数，`MapScenarioModeCheck` 有 8 个 `unsigned char*` 参数
4. **全局状态**：`static` 全局变量（如 `bEndCarPosInitFlag`, `bFindLaneLineFlag` 等）散布在多个函数中
5. **大量魔数参数**：`unsigned char` 类型参数表示枚举值但没有明确的枚举类型

### 6.2 建议的重构方向

#### Phase 1：接口梳理（无源码时也能做）
- 对照函数列表和调用关系，绘制模块依赖图
- 标注每个函数的数据来源和去向
- 确认全局变量的读写分布

#### Phase 2：代码还原（拿到原始源码后）
1. **类封装**：将全局函数重构为 `MapParkingOut` 类的方法
2. **状态变量**：将散布的 `static` 变量收拢为成员变量
3. **枚举化**：将 `unsigned char` 参数替换为 `enum class`
4. **参数聚合**：将 8+ 个参数的函数拆分为结构体传参
5. **Pipeline 模式**：将传感器数据到最终输出的流程抽象为明确的 pipeline 阶段

#### Phase 3：单元测试
- 为每个传感器数据处理函数编写独立测试
- 为边界融合函数编写场景测试（垂直/平行/斜向）
- 为终车位置计算编写回归测试

### 6.3 重构优先级

```
优先级 1: 全局变量整理（最危险，最易引入 bug）
优先级 2: 场景模式判断逻辑提取
优先级 3: 传感器数据筛选排序函数简化
优先级 4: 车位构建函数拆分
优先级 5: 边界融合函数参数聚合
```

---

## 七、调试技巧

### 7.1 编译选项
CMake 中已启用 `ENABLE_PERF`，可查看函数耗时。如需更多调试：
```bash
# Debug 编译
cmake -DCMAKE_BUILD_TARGET=x86_debug ..
make -j$(nproc)

# 或给 Release 添加 -g 符号
cmake -DCMAKE_BUILD_TARGET=x86_release ..  # CMakeLists 中已含 -g
```

### 7.2 GDB 断点设置
```gdb
# 在特定函数入口断点
b APAMap_ParkingOutTask
b APAMap_ParkingOutCalSlotInfo
b APAMap_ParkingOutFusBoundaryByPDCInfo

# 条件断点（如某标志位为真）
b APAMap_ParkingOutEndCarPosUpdata if bEndCarPosInitFlag != 0
```

### 7.3 日志建议
在关键 pipeline 阶段加入结构化日志：
```
[ParkingOut] SlotType=PARALLEL Mode=3 Side=LEFT
[ParkingOut] FusSource=PDC+SDG BkupCount=2
[ParkingOut] EndPos x=1.23 y=4.56 yaw=0.78
```

---

## 八、调用链分析摘要

### 主调用链
```
APAMap_ParkingOutTask()
 ├─ APAMap_ParkingOutBkDataBfSDGFusInit()      // 初始化备份
 ├─ APAMap_ParkingOutBkSDGOutPutDataInit()
 ├─ APAMap_ParkingOutCalMapInfo()               // 核心计算
 │   ├─ APAMap_ParkingOutMapScenarioModeCheck() // 场景判断
 │   ├─ APAMap_ParkingOutCalSlotInfo()           // 车位计算
 │   │   ├─ 传感器数据获取
 │   │   ├─ 车位构建
 │   │   └─ 车位参数整理
 │   └─ APAMap_ParkingOutFusBoundaryByXxx()     // 边界融合
 ├─ APAMap_ParkingOutUpDataMapInfo()             // 更新输出
 └─ 数据保存/备份
```

### 与外部模块的接口
```
AlgCom_SetParkingOutObjAng()                    // 被其他模块调用设置目标角度
HybridAStar::CheckParkingOutReachRelaxEnd()     // 混合A*规划器调用，判断是否到达松弛终点
ParkingOutRelaxEnd (protobuf)                   // 泊出松弛终点的 protobuf 消息定义
```
