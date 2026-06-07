# MapParkingOut.cpp 解析报告

**文件路径:** `src/pnc_map/MapParkingOut.cpp`
**总行数:** 15,620 行
**源码状态:** 磁盘文件已被二进制数据覆盖，已从 git 恢复原始源码
**生成时间:** 2026-05-11

---

## 1. 全局变量 (20个)

包级别的状态标志位和数据结构，用于记录泊出过程中的各种状态：

| 变量名 | 类型 | 说明 |
|--------|------|------|
| `bCntAddFlag` | BOOLEAN | cnt+1轨迹重算标志，下一帧清空 |
| `bLaneLineUpdateEndCarPosFlag` | BOOLEAN | 车道线更新终点位置标志 |
| `bRefercLineUpdateEndCarPosFlag` | BOOLEAN | 车位参考线更新终点位置标志 |
| `bAfterNewAnchorPointFlag` | BOOLEAN | 锚点转换后标志位 |
| `bFsdInRightOfEndCarPosFlag` | BOOLEAN | FSD点位入侵终点位置右边标志 |
| `bFsdFromMapMainSlotBordFlag` | BOOLEAN | 入侵的边界点是否来自主边界标志位 |
| `bFsdFromMapSubSlotBordFlag` | BOOLEAN | 入侵的边界点是否来自子边界标志位 |
| `bFsdFromMapMainAndSubSlotBordFlag` | BOOLEAN | 入侵的边界点是否来自主子边界标志位 |
| `bPreventStepNRedundantFlag` | BOOLEAN | 防多走标志位 |
| `bShortestSlotLen` | BOOLEAN | 水平极小车位标志 |
| `bShortSlotLen` | BOOLEAN | 水平小车位标志 |
| `bLongestSlotLen` | BOOLEAN | 水平极大车位标志 |
| `bCarryOutSlot` | BOOLEAN | 采用车位框标志位 |
| `bLabelAngledFlag` | BOOLEAN | 斜列车位框标志 |
| `bObjLabelLadderFlag` | BOOLEAN | 斜列阶梯车位框标志 |
| `bLabelAngledParkingOutSlotFlag` | BOOLEAN | 斜列车位泊出车位后标志位 |
| `bODWheelChockFlag` | BOOLEAN | 水平泊出车位内有轮挡标志 |
| `APAMap_BkDataBfSDGFus` | tMap_MapBkInfo_BeForeFusSDG_t | 融合前SDG边界备份数据 |
| `APAMap_BkSDGOutPutData` | tMap_MapBkInfo_SDGBkOutPutData_t | SDG边界输出备份数据 |
| `ParkOutEightMode` | tAPAParkProcEightParkingOutModeType | 八种泊出模式类型 |

---

## 2. 函数列表 (17个实际函数)

| 行号范围 | 行数 | 函数名 | 返回类型 | 功能说明 |
|----------|------|--------|----------|----------|
| L31-100 | 70 | `APAMap_ParkingOutTask` | void | **主入口任务函数** |
| L101-242 | 142 | `APAMap_ParkingOutCalMapSlotPar` | BOOLEAN | 计算地图车位参数 |
| L243-273 | 31 | `APAMap_ParkingOutCalSlotInfo` | BOOLEAN | 计算车位信息 |
| L274-525 | 252 | `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo` | BOOLEAN | 根据FSD和OD地图信息计算车位边界点 |
| L978-1401 | 424 | `APAMap_ParkingOutCalSlotBorderPtByParkOutInfo` | BOOLEAN | 根据泊出信息计算车位边界点 |
| L1402-1502 | 101 | `APAMap_ParkingOutCalMapInfo` | BOOLEAN | 计算地图信息 |
| L1503-1559 | 57 | `APAMap_ParkingOutCheckIfCarPosIsValid` | BOOLEAN | 检查车位位置是否有效 |
| L1560-1999 | 440 | `APAMap_ParkingOutCalBoundaryByParkOutInfo` | bool_t_INF | 根据泊出信息计算边界 |
| L2000-2391 | 392 | `APAMap_ParkingOutFusBoundaryByFSDMapInfo` | BOOLEAN | FSD地图信息融合边界 |
| L2735-2939 | 205 | `APAMap_ParkingOutFusBoundaryByLaneLineMapInfo` | BOOLEAN | 车道线地图信息融合边界 |
| L3358-3784 | 427 | `APAMap_ParkingOutFusBoundaryByRefercLineMapInfo` | BOOLEAN | 参考线地图信息融合边界 |
| L4555-4618 | 64 | `APAMap_ParkingOutUpDataMapInfo` | void | 更新地图信息 |
| L4619-5214 | 596 | `APAMap_ParkingOutCalSlotSlotAlignInfo` | void | 计算车位对齐信息 |
| L5215-5427 | 213 | `APAMap_ParkingOutbWideChannelFlag` | BOOLEAN | 判断是否宽通道标志 |
| L5428-5673 | 246 | `APAMap_ParkingOutCenterEndCarPosInfo` | BOOLEAN | 计算居中终点位置信息 |
| L5674-7254 | 1581 | `APAMap_ParkingOutBoundarySeizeEndCarPosInfo` | BOOLEAN | **边界侵占终点位置处理（最大函数）** |
| L7255-7310 | 56 | `APAMap_ParkingOutUpDataMapInfoBySlotCorInfo` | BOOLEAN | 根据车位纠正信息更新地图 |
| L7311-7428 | 118 | `APAMap_ParkingOutObliqueRowStairsInfo` | BOOLEAN | 斜列阶梯车位信息 |
| L12084-13545 | 1462 | `APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo` | BOOLEAN | 根据泊出车位信息计算边界 |
| L14185-15301 | 1117 | `APAMap_ParkingOutGetEightParkOutMode` | tAPAParkProcEightParkingOutModeType | 获取八种泊出模式 |

---

## 3. 核心调用流程 (APAMap_ParkingOutTask 主流程)

```
APAMap_ParkingOutTask()
│
├── 条件A: APARunningstate >= 4 && Request_cmd == 1 && request_cnt == lastreqcnt
│   └── APAMap_ParkingOutUpDataMapInfo()  // 复用上次结果，快速返回
│
├── 首次请求 (Request_cmd == 1 或 6)
│   ├── Step1: APAMap_ParkingOutCalMapSlotPar()     // 计算车位参数
│   │   - 解析 parkoutmode / parkside / SlotID / FusionMode
│   │   - 初始化 SlotPar.VplPt/UsPt/ODPt/FSDPt/PAPt = NO_OBJ_DISTANCE
│   │   - 校验 SlotID 是否有效，无效 → failcause(1)
│   │   - 在 USS/VPL/FusSlot 中查找匹配车位，无 → failcause(2)
│   │
│   ├── Step2: APAMap_ParkingOutCalSlotInfo()        // 计算车位信息
│   │   - APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo() [ifdef]
│   │   - APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()     // 主计算路径
│   │   - APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo() // 融合边界
│   │
│   ├── Step3: APAMap_ParkingOutCalMapInfo()         // 计算地图信息（核心融合）
│   │   - APAMap_ParkingOutCalBoundaryByParkOutInfo()
│   │   - APAMap_ParkingOutFusBoundaryByFSDMapInfo()
│   │   - APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()
│   │   - APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()
│   │   - APAMap_FusBoundaryByODMapInfo()
│   │   - APAMap_SmoothMapBoundary()
│   │   - SDG/PDC 融合 (ifdef)
│   │   - APAMap_ParkingOutBoundarySeizeEndCarPosInfo()
│   │   - APAMap_ParkingOutCenterEndCarPosInfo()
│   │   失败 → failcause(45)
│   │
│   └── Step4: APAMap_ParkingOutCheckIfCarPosIsValid() // 校验车位有效性
│       失败 → failcause(47/48/49)
│
├── 非首次请求 (边界点数不足 < 2)
│   ├── APAMap_DataInit()
│   ├── APAMap_ParkingOutDebugInit()
│   ├── Setfailcause(59)
│   └── APAMap_ParkingOutUpDataMapInfo()
│
└── APARunningstate >= 7
    └── APAMap_ParkingOutDebugInit()
```

---

## 4. 失败码 (FailCause) 定义

定义在 `MapType.h` 的 `tAPATrajCalFailCauseDataType` 枚举中：

| 失败码 | 含义 | 触发条件 |
|--------|------|----------|
| 1 | 请求计算失败 | 车位ID为无效 (`SlotID == APA_VPL_SLOT_PROC_INVALID_SLOT_ID`) |
| 2 | 地图构建失败 | 未找到匹配的融合车位 |
| 45 | 地图信息计算失败 | `APAMap_ParkingOutCalMapInfo()` 返回 FALSE |
| 47 | 车辆位置无效 | 车位坐标系下车身角度 \|CarAng\| > 90° |
| 48 | 车辆位置无效 | 左前/右前角点 x 坐标超出车身半宽 |
| 49 | 车辆位置无效 | 左后/右后角点 x 坐标 < -7000 |
| 57 | 边界计算失败 | 垂直泊出模式下车位长度过小 |
| 58 | 边界计算失败 | 水平泊出模式下车位宽度过小 |
| 59 | 地图边界不足 | 左右边界点数 < 2 |
| 100 | 泊出专用错误 | 内部校验失败 |
| 101 | 泊出专用错误 | 内部校验失败（6次调用） |

---

## 5. 泊出模式枚举

### 基础泊出模式 (tAPAParkProcParkingOutModeType)

| 值 | 模式名 | 说明 |
|----|--------|------|
| 0 | HEAD_TURN_ROUND | 车头原地掉头 |
| 1 | HEAD_GO_STRAIGHT | 车头直行 |
| 2 | REAR_TURN_ROUND | 车尾原地掉头 |
| 3 | REAR_GO_STRAIGHT | 车尾直行 |
| 4 | PARALLEL | 平行泊出 |
| 5 | UNKNOWNMODE | 未知模式 |

### 八种精细泊出模式 (tAPAParkProcEightParkingOutModeType)

| 值 | 模式名 | 说明 |
|----|--------|------|
| 0 | HEAD_GO_STRAIGHT | 车头直行出 |
| 1 | REAR_GO_STRAIGHT | 车尾直行出 |
| 2 | HEAD_PARALLEL_LEFT | 平行车位向左出 |
| 3 | HEAD_PARALLEL_RIGHT | 平行车位向右出 |
| 4 | HEAD_PERP_LEFT | 垂直车位头向左侧出 |
| 5 | HEAD_PERP_RIGHT | 垂直车位头向右侧出 |
| 6 | REAR_PERP_LEFT | 垂直车位尾向左侧出 |
| 7 | REAR_PERP_RIGHT | 垂直车位尾向右侧出 |
| 8 | UNKNOWNMODE | 未知 |

---

## 6. APAMap_ParkingOutCalMapInfo() 核心流程

最核心的地图融合函数，完整调用链：

```
APAMap_ParkingOutCalMapInfo()
│
├─ APAMap_ParkingOutCalBoundaryByParkOutInfo()
│   → 基于泊出信息计算初始边界
│   BoudaryNum[0] 记录点数
│
├─ APAMap_ParkingOutFusBoundaryByFSDMapInfo()
│   → FSD自由空间检测融合边界
│   BoudaryNum[1] 记录点数
│   debug3++
│
├─ APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()
│   → 车道线信息融合边界（仅锚点转换后）
│   BoudaryNum2[2] 记录
│
├─ APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()
│   → 参考线信息融合边界（仅锚点转换后）
│   BoudaryNum2[1] 记录
│
├─ APAMap_FusBoundaryByODMapInfo()
│   → OD（障碍物检测）融合边界
│   BoudaryNum[2] 记录点数
│   debug4++
│
├─ APAMap_SmoothMapBoundary(0)  → 边界平滑
│
├─ #ifdef APAMAP_PARKOUT_FUS_SDG
│   APAMap_ParkingOutUpDataMapBoundaryBySDGInfo()
│   APAMap_ParkingOutDeleteMainSlotBord()
│
├─ #ifdef APAMAP_PARKOUT_FUS_PDC
│   APAMap_ParkingOutUpDataMapBoundaryByPDCInfo()
│   APAMap_ParkingOutDeleteMainSlotBord()
│
├─ APAMap_SmoothMapBoundary(0)  → 再次平滑
│   BoudaryNum[3] 记录点数
│   debug2++
│
├─ APAMap_ParkingOutBoundarySeizeEndCarPosInfo()
│   → 判断边界点是否侵占终点位置
│   → bSeizeEndCarPosFlag = TRUE/FALSE
│
├─ APAMap_ParkingOutCenterEndCarPosInfo()
│   → 如果侵占，则计算居中的终点位置
│   → 再次调用 BoundarySeize 判断
│
└─ EndPos 最终写入 APAMap_GInfo.SlotPar.EndPos
```

---

## 7. APAMap_ParkingOutCheckIfCarPosIsValid() 车辆位置校验

校验逻辑（车辆必须在车位坐标系中）：

```
车辆位置有效条件:
1. parkmode == PARKING_OUT / PARKEXIT → 直接返回 TRUE（不做校验）
2. |CarAng| <= 90° → 否则 failcause(47)
3. 前轴角点 Ptcc[0/1] 的 x <= HalfWidthOfCar → 否则 failcause(48)
4. 后轴角点 Ptcc[2/3] 的 x >= -7000 → 否则 failcause(49)

角点计算方式:
Ptcc[0]: 左前角 (x=HalfWidthOfCar, y=LenBetweenRAxisAndFBumper)
Ptcc[1]: 右前角 (x=HalfWidthOfCar, y=-LenBetweenRAxisAndRBumper)
Ptcc[2]: 左后角 (x=-HalfWidthOfCar, y=LenBetweenRAxisAndFBumper)
Ptcc[3]: 右后角 (x=-HalfWidthOfCar, y=-LenBetweenRAxisAndRBumper)
```

---

## 8. APAMap_ParkingOutUpDataMapInfo() — 地图更新（复算路径）

当 `request_cnt == lastreqcnt` 时走此路径，复用上次计算结果：

```
APAMap_ParkingOutUpDataMapInfo()
│
├─ #ifdef APAMAP_PARKOUT_FUS_SDG
│   APAMap_ParkingOutSetMainSlotBordInfoByBkDataBfSDGFus()
│   恢复备份数据
│
├─ #ifdef APAMAP_PARKOUT_FUS_PDC
│   APAMap_ParkingOutSetSlotBordInfoByBkDataBfPDCFus()
│
├─ APAMap_ParkingOutUpDataMapInfoBySlotCorInfo()
│   BoudaryNum[5] 记录
│
├─ APAMap_ParkingOutSideSlotInfo()
│   判断是否需要更新边界（宽通道场景）
│
├─ 如果 bUpdataCalBoundaryFlag == TRUE
│   APAMap_ParkingOutCalBoundaryByParkOutInfo()
│   BoudaryNum2[0] 记录
│
├─ APAMap_ParkingOutFusBoundaryByFSDMapInfo()
│   BoudaryNum[6] 记录
│
├─ 如果 bAfterNewAnchorPointFlag == TRUE
│   ├─ APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()
│   │   BoudaryNum2[2] 记录
│   └─ APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()
│       BoudaryNum2[1] 记录
│
├─ APAMap_FusBoundaryByODMapInfo()
│   BoudaryNum[7] 记录
│
├─ mode = (左边界不变 ? 0x02 : 0) | (右边界不变 ? 0x01 : 0)
│   APAMap_SmoothMapBoundary(mode)
│
├─ #ifdef APAMAP_PARKOUT_FUS_SDG
│   APAMap_ParkingOutUpDataMapBoundaryBySDGInfo()
│   APAMap_ParkingOutDeleteMainSlotBord()
│
├─ #ifdef APAMAP_PARKOUT_FUS_PDC
│   APAMap_ParkingOutUpDataMapBoundaryByPDCInfo()
│   APAMap_ParkingOutDeleteMainSlotBord()
│
├─ APAMap_SmoothMapBoundary(mode)
│   BoudaryNum[8] 记录
│
└─ APAMap_ParkingOutEndCarPosUpdata()
```

---

## 9. 关键状态标志位详解

| 标志位 | 含义 | 使用场景 |
|--------|------|----------|
| `bAfterNewAnchorPointFlag` | 锚点是否已转换 | 锚点转换后开启车道线/参考线融合 |
| `bFsdInRightOfEndCarPosFlag` | FSD点是否入侵终点右侧 | 控制终点位置是否需要调整 |
| `bPreventStepNRedundantFlag` | 防多走 | 防止车辆在阶梯车位中过度移动 |
| `bWideChannelFlag` | 宽通道场景 | 宽通道下采用通道PDC融合边界 |
| `bCarryOutSlot` | 采用车位框 | 是否使用车位框而非自由边界 |
| `bLabelAngledFlag` | 斜列车位框 | 斜列车位检测 |
| `bObjLabelLadderFlag` | 阶梯车位框 | 阶梯斜列/斜列阶梯检测 |
| `bODWheelChockFlag` | 车位内有轮挡 | 水平泊出特殊处理 |
| `bSlotDataAtRigthSide` | 车位数据在右侧 | 控制左右镜像逻辑 |

---

## 10. 数据输入来源汇总

| 数据类型 | 来源 | 用途 |
|----------|------|------|
| USS超声波车位 | `APAMap_GInputData.Usslot` | 车位检测定位 |
| VPL视觉车位 | `APAMap_GInputData.Vplslot` | 视觉融合车位 |
| 融合车位 | `APAMap_GInputData.FusSlot` | USS+VPL融合结果 |
| FSD自由空间 | `APAMap_GInputData.TopViewInfo` | 边界融合 |
| 车道线 | `APAMap_GInputData.LaneLineInfo` | 车道级参考 |
| 参考线 | `APAMap_GInputData.RefercLineInfo` | 路径参考 |
| OD障碍物 | `APAMap_GInputData.ODObjects` | 障碍物检测 |
| 车辆位置 | `APAMap_GInputData.CarLocInfo` | 定位信息 |

---

## 11. 坐标系与坐标变换

- **车位坐标系**: 原点为 `NewCordSysOPt`，航向角 `NewCordSysAng`
- **旧坐标系**: 原始全局坐标系
- **过渡/临时坐标系**: 角点计算时用 `AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys` 转换
- **左右镜像**: `bSlotDataAtRigthSide` 控制是否需要镜像数据
- **角度归一化**: `AlgCom_AngNormalized(&CarAng)` 确保角度在 [-π, π]

---

## 12. BoudaryNum / BoudaryNum2 调试数组

| 数组 | 记录阶段 | 说明 |
|------|----------|------|
| `BoudaryNum[0]` | 初始边界 | CalBoundaryByParkOutInfo 后 |
| `BoudaryNum[1]` | FSD融合后 | FusBoundaryByFSDMapInfo 后 |
| `BoudaryNum[2]` | OD融合后 | FusBoundaryByODMapInfo 后 |
| `BoudaryNum[3]` | 平滑后 | 最后平滑后 |
| `BoudaryNum[4]` | 更新前 | UpDataMapInfo 开始 |
| `BoudaryNum[5]` | SlotCor后 | UpDataMapInfoBySlotCorInfo 后 |
| `BoudaryNum[6]` | FSD融合后 | UpDataMapInfo 中 |
| `BoudaryNum[7]` | OD融合后 | UpDataMapInfo 中 |
| `BoudaryNum[8]` | 最终 | UpDataMapInfo 结束 |
| `BoudaryNum2[0]` | 侧边更新后 | SideSlotInfo 后 |
| `BoudaryNum2[1]` | 参考线融合后 | FusBoundaryByRefercLineMapInfo 后 |
| `BoudaryNum2[2]` | 车道线融合后 | FusBoundaryByLaneLineMapInfo 后 |

---

## 13. 车位边界点类型 (SlotPar 中的点)

| 点类型 | 含义 | 说明 |
|--------|------|------|
| `VplPt` | 视觉车位点 | 来自VPL视觉车位 |
| `UsPt` | 超声波车位点 | 来自USS超声波 |
| `ODPt` | 障碍物检测点 | 来自OD障碍物检测 |
| `FSDPt` | 自由空间检测点 | 来自FSD |
| `PAPt` | 泊出辅助点 | 泊出专用辅助点 |

---

## 14. 重要编译宏

| 宏 | 说明 |
|----|------|
| `SUPPORT_PARKING_OUT_SYSTEM` | 启用泊出系统 |
| `APAMAP_PARKOUT_USE_SDG_OBJS` | 使用SDG目标物融合 |
| `APAMAP_PARKOUT_FUS_SDG` | 融合SDG边界 |
| `APAMAP_PARKOUT_FUS_PDC` | 融合PDC边界 |
| `APA_MAP_PARK_OUT_WITH_VPLSLOTPTS_FROM_TOTALMAPINFO` | 从总地图获取VPL车位点 |
| `APA_MAP_PARKOUT_LADDER_SLOT` | 支持阶梯车位 |
| `SUPPORT_PARKING_OUT_DEBUG` | 调试信息开关 |

---

## 15. debug 计数器

| 计数器 | 增量位置 | 说明 |
|--------|----------|------|
| `debug1++` | CalSlotInfo → FSDAndOD 融合后 | 车位信息计算成功 |
| `debug2++` | CalMapInfo → 平滑后 | 地图信息计算成功 |
| `debug3++` | CalMapInfo → FSD融合后 | FSD融合成功 |
| `debug4++` | CalMapInfo → OD融合后 | OD融合成功 |
| `debug5++` | - | 未在 ParkingOut.cpp 中使用 |

---

## 16. 文件头注释要点

以下文件磁盘内容被二进制覆盖，建议立即恢复：

- `src/pnc_map/MapParkingOut.cpp` (552KB → 二进制 TSD 格式)
- `src/pnc_map/MapParkingOut.h` (11KB → 二进制 TSD 格式)

**恢复命令:**

```bash
git checkout HEAD -- src/pnc_map/MapParkingOut.cpp src/pnc_map/MapParkingOut.h
```

**git 历史中存储正常，最近一次提交:**

```
f93d7b5a fix()：更新版本4.10.53
```
