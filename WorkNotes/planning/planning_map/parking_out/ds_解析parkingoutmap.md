# MapParkingOut.cpp 代码解析文档

> 源文件: `/media/disk/2818_proj/perf_opt/src/pnc_map/MapParkingOut.cpp`
> 最新提交: `f93d7b5a fix()：更新版本4.10.53`
> 解析日期: 2026-05-11

---

## 1. 文件概览

| 属性 | 值 |
|------|-----|
| 总行数 | 15,619 |
| 模块功能 | APA（自动泊车辅助）完全泊出建图模块 |
| 所属系统 | PNC Map 子系统 |
| 编译条件 | `#ifdef SUPPORT_PARKING_OUT_SYSTEM` |
| 核心职责 | 根据车位信息、传感器感知数据（SDG/PDC/FSD/OD）、车道线等，构建泊出路径的左右边界 |

### 关联头文件

| 头文件 | 用途 |
|--------|------|
| `Map.h` | 地图核心数据结构 |
| `APAMapCfg.h` | APA 地图配置参数 |
| `AlgCom.h` | 通用算法库（几何计算） |
| `MapType.h` | 地图类型定义 |
| `MapParkingOut.h` | 泊出模块对外接口 |
| `data_exchange/someip/planning_data_interface.h` | SOME/IP 数据交换 |
| `Map_DeadendScenario_Decider.h` | 死胡同场景决策器 |
| `common/log_wrap.h` | 日志封装 |

---

## 2. 整体架构

### 2.1 主调用链

```
APAMap_ParkingOutTask()                     [L31-99]   主入口
├── APAMap_ParkingOutDebugInit()            [L945]     初始化/复位
├── APAMap_ParkingOutCalMapSlotPar()        [L101]     步骤1: 计算地图-车位参数
│   └── 读取ParkReqPar: parkoutmode, SlotID, parkside, FusionMode
│   └── 初始化SlotPar的VplPt/UsPt/ODPt/FSDPt/PAPt为NO_OBJ_DISTANCE
│   └── 确定bSlotDataAtRigthSide（数据镜像侧）
├── APAMap_ParkingOutCalSlotInfo()          [L243]     步骤2: 计算车位边界信息
│   ├── APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()
│   ├── APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()
│   └── APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo() [L274]
│       ├── AlgCom_SetParkOutObj1Pt()        [L785]   Obj1点计算
│       ├── AlgCom_SetParkOutObj2Pt()        [L852]   Obj2点计算
│       └── AlgCom_SetParkingOutObjAng()     [L919]   Obj角度计算
├── APAMap_ParkingOutCalMapInfo()           [L1402]    步骤3: 构建完整边界
│   ├── APAMap_ParkingOutCalBoundaryByParkOutInfo()    从泊出信息计算边界
│   ├── APAMap_ParkingOutFusBoundaryByFSDMapInfo()     FSD地图边界融合
│   ├── APAMap_ParkingOutFusBoundaryByLaneLineMapInfo() 车道线边界融合
│   ├── APAMap_ParkingOutFusBoundaryByRefercLineMapInfo() 参考线边界融合
│   └── APAMap_FusBoundaryByODMapInfo()                OD障碍物边界融合
├── APAMap_ParkingOutCheckIfCarPosIsValid()            步骤4: 车辆位姿校验
├── APAMap_ParkingOutEndCarPosUpdata()      [L6405]    终点位置动态更新
├── APAMap_ParkingOutPickDispersedObstacles()[L6891]   离散障碍物筛选
├── APAMap_ParkingOutUpDataMapInfo()                   地图信息刷新输出
│
├── [SDG 融合分支]
│   ├── APAMap_ParkingOutBkDataBfSDGFusInit()  [L12289]
│   ├── APAMap_ParkingOutBkSDGOutPutDataInit() [L12295]
│   ├── APAMap_ParkingOutCheckIfFusBoundarySDGInfo() [L13128]
│   ├── APAMap_ParkingOutUpDataMapBoundaryBySDGInfo() [L13088]
│   ├── APAMap_ParkingOutSetMainSlotBordInfoByBkDataBfSDGFus() [L12921]
│   ├── APAMap_ParkingOutDeleteMainSlotBord()  [L12955]
│   └── APAMap_ParkingOutSaveBkDataBfSDGFus() [L13055]
│
├── [PDC 融合分支]
│   ├── APAMap_ParkingOutCheckIfFusBoundaryPDCInfo() [L14408]
│   ├── APAMap_ParkingOutUpDataMapBoundaryByPDCInfo() [L14361]
│   ├── APAMap_ParkingOutFusBoundaryByPDCInfo() [L15018]
│   ├── APAMap_ParkingOutSetSlotBordInfoByBkDataBfPDCFus() [L14312]
│   └── APAMap_ParkingOutSaveBkDataBfPDCFus() [L14971]
│
└── APAMap_ParkingOutSideSlotInfo()          [L14252]  侧向车位信息
```

### 2.2 数据流图 (简化)

```
ParkReqPar (请求参数)
    ↓
[CalMapSlotPar] → SlotPar 初始化 (SlotID, ParkSide, SlotSide, bSlotDataAtRigthSide)
    ↓
[CalSlotInfo] → 车位边界点 (Obj1Pt, Obj2Pt, ObjAng, SlotLen)
    ↓              FSDOffset, ODOffset → 终点位置(EndPos)
    ↓
[CalMapInfo] → 泊出通道左右边界 (LeftBoundary, RightBoundary)
    ↓              ↘ SDG信息融合 (通道级语义)
    ↓               ↘ PDC信息融合 (超声传感器)
    ↓               ↘ LaneLine融合  (车道线)
    ↓               ↘ RefercLine融合 (车位参考线)
    ↓               ↘ OD融合 (障碍物)
    ↓
[EndCarPosUpdata] → 动态终点位置修正（车辆移动后重新计算）
    ↓
[PickDispersedObstacles] → 离散障碍物筛选处理
    ↓
[UpDataMapInfo] → 输出结果写入 APAMap_GInfo.OutLine
```

---

## 3. 全局标志位说明

文件中定义了 **27 个模块级 BOOLEAN 标志位**，用于控制泊出建图各阶段的行为：

| 标志位 | 行号 | 含义 |
|--------|------|------|
| `bCntAddFlag` | L11 | cnt+1 轨迹重算标志位，下一帧会清空为 FALSE |
| `bLaneLineUpdateEndCarPosFlag` | L12 | 车道线更新终点位置标志位 |
| `bRefercLineUpdateEndCarPosFlag` | L13 | 车位参考线更新终点位置标志位 |
| `bAfterNewAnchorPointFlag` | L14 | 锚点转换后标志位（已驶出原车位坐标系） |
| `bFsdInRightOfEndCarPosFlag` | L15 | FSD 点位入侵终点位置右边标志位 |
| `bFsdFromMapMainSlotBordFlag` | L16 | 入侵边界点来自主边界标志位 |
| `bFsdFromMapSubSlotBordFlag` | L17 | 入侵边界点来自子边界标志位 |
| `bFsdFromMapMainAndSubSlotBordFlag` | L18 | 入侵边界点来自主子边界标志位 |
| `bPreventStepNRedundantFlag` | L19 | 防多走标志位 |
| `bShortestSlotLen` | L20 | 水平极小车位标志位 |
| `bShortSlotLen` | L21 | 水平小车位标志位 |
| `bLonggestSlotLen` | L22 | 水平极大车位标志位 |
| `bCarryOutSlot` | L23 | 采用车位框标志位 |
| `bLabelAngledFlag` | L24 | 斜列车位框标志位 |
| `bObjLabelLadderFlag` | L25 | 斜列阶梯车位框标志位 |
| `bLabelAngledParkingOutSlotFlag` | L26 | 斜列车位泊出车位后标志位 |
| `bODWheelChockFlag` | L27 | 水平泊出车位内有轮挡标志位 |
| `bCenterEndCarPosFlag` | L1410 | 采用终点位置居中标志位 |
| `bWideChannelFlag` | L5235 | 宽通道场景标志位 |

### 关键全局数据结构

```c
tMap_MapBkInfo_BeForeFusSDG_t  APAMap_BkDataBfSDGFus;      // SDG融合前备份数据
tMap_MapBkInfo_SDGBkOutPutData_t APAMap_BkSDGOutPutData;    // SDG备份输出数据
tAPAParkProcEightParkingOutModeType ParkOutEightMode;        // 八种泊出模式
```

---

## 4. 核心函数详解

### 4.1 APAMap_ParkingOutTask() — 主入口 [L31-99]

```c
void APAMap_ParkingOutTask()
```

**功能**: 泊出建图的主调度函数，每帧调用一次。

**流程**:
1. 如果 APA 运行状态 ≥ 4 且请求为重复帧（request_cnt 未变），仅执行 `APAMap_ParkingOutUpDataMapInfo()` 刷新输出
2. 如果请求为 cmd=1（建图）或 cmd=6（重新建图）:
   - 执行首次建图日志记录
   - `APAMap_GInfo.calcnt++`
   - APA 状态 ≤ 3 且运行状态 ≥ 1 → 调用 `APAMap_ParkingOutDebugInit()`
   - 串行执行三步建图流程: CalMapSlotPar → CalSlotInfo → CalMapInfo
   - 任一步失败 → `APAMAP_Setfailcause(45)`
   - 全部成功后校验 `CheckIfCarPosIsValid`
3. 非首次建图帧: 检查左右边界点数是否 ≥ 2，不足则 `Setfailcause(59)`
4. APA 运行状态 ≥ 7 → DebugInit

### 4.2 APAMap_ParkingOutCalMapSlotPar() — 车位参数计算 [L101-241]

```c
BOOLEAN APAMap_ParkingOutCalMapSlotPar()
```

**功能**: 从请求参数解析车位信息，初始化 SlotPar 结构体。

**关键逻辑**:
- 读取 `parkoutmode` 确定泊出模式: 水平(`PARALLEL`)/非水平
- 读取 `parkside` 确定泊车侧: 左侧(`LEFT_SIDE`)/右侧
- 读取 `SlotDataIsNotMirrored` 确定 `bSlotDataAtRigthSide`
- 将 VplPt/UsPt/ODPt/FSDPt/PAPt 全部初始化为 `NO_OBJ_DISTANCE`
- 设置 CarPos、SlotID、SlotIndex、FusSlotIndex 等参数
- 根据 FusionMode 选择融合车位索引

### 4.3 APAMap_ParkingOutCalSlotInfo() — 车位边界信息计算 [L243-261]

```c
BOOLEAN APAMap_ParkingOutCalSlotInfo()
```

**功能**: 计算车位边界点（Obj1Pt、Obj2Pt 等），为后续边界生成提供基准。

**优先级链**:
1. `ParkOutSlotInfo` → 从车位信息直接计算
2. `ParkOutInfo` → 从泊出信息计算
3. `FSDAndODMapInfo` → 从 FSD（自由空间检测）+ OD（障碍物检测）地图信息计算

### 4.4 APAMap_ParkingOutCalMapInfo() — 地图边界构建 [L1402-1725]

```c
BOOLEAN APAMap_ParkingOutCalMapInfo()
```

**功能**: 构建泊出通道的左右边界，多传感器融合。

**融合叠代顺序**:
1. `CalBoundaryByParkOutInfo()` — 从泊出信息生成初始边界
2. `FusBoundaryByFSDMapInfo()` — FSD 地图边界融合
3. `FusBoundaryByLaneLineMapInfo()` — 车道线边界融合
4. `FusBoundaryByRefercLineMapInfo()` — 车位参考线融合
5. `FusBoundaryByODMapInfo()` — OD 障碍物融合

每步融合后记录 `BoudaryNum[i][0/1]` 用于调试追踪边界点数变化。

### 4.5 APAMap_ParkingOutEndCarPosUpdata() — 终点动态更新 [L6405-6890]

```c
void APAMap_ParkingOutEndCarPosUpdata()
```

**功能**: 车辆运动过程中动态修正终点位置。

**关键逻辑**:
- 检测 `bAfterNewAnchorPointFlag`：锚点转换后不再更新 Obj1/Obj2
- 根据泊出模式判断车辆是否已驶出车位 (`bInsideSlotFlag`)
- 若车辆 x 轴坐标 > 1m 且与终点 x 偏差 > 200mm → 停止更新 Obj 点
- 根据宽通道场景 (`bWideChannelFlag`) 调整 Offset

### 4.6 APAMap_ParkingOutPickDispersedObstacles() — 离散障碍物筛选 [L6891-7254]

```c
void APAMap_ParkingOutPickDispersedObstacles(ObstaclesInfo_INF* pObjInfo)
```

**功能**: 从 TotalMapInfo 的 OD 信息中筛选与泊出相关的障碍物。

**处理流程**:
1. 调用 `APAMap_CalAndAddRskOBjObstacles()` 添加风险障碍物
2. 遍历 `TotalMapInfo.mapData.ODInfo`
3. 根据泊出模式和车辆终点位置构建多个矩形检测区
4. 障碍物落入检测区 → 添加到 pObjInfo 输出

### 4.7 SDG 融合流程

```
CheckIfFusBoundarySDGInfo() → 判断是否可融合 SDG 边界
   条件: SDG 信息有效、Obj 点在线段内等
     ↓
UpDataMapBoundaryBySDGInfo() → 用 SDG 信息更新地图边界
   1. GetSDGInfoPt() — 提取 SDG 主/子边界点
   2. SetMainSlotBordInfoByBkDataBfSDGFus() — 用备份数据恢复边界
   3. FusBoundaryBySDGInfo() — 将 SDG 点融合进当前边界
   4. SaveBkDataBfSDGFus() — 保存当前边界为下次备份
```

SDG（Semantic Data Graph，语义数据图）提供通道级别的结构化感知信息，用于在宽阔场景下生成稳定的泊出通道边界。

### 4.8 PDC 融合流程

```
CheckIfFusBoundaryPDCInfo() → 判断是否可融合 PDC 边界
   条件: SDG 状态为 Keep/Updata 时启用 PDC
     ↓
UpDataMapBoundaryByPDCInfo() → 用 PDC 信息更新地图边界
   1. GetPDCInfoPt() — 提取 PDC 主/子边界点
   2. SaveBkDataBfPDCFus() — 备份当前主/子边界
   3. FusBoundaryByPDCInfo() — 将 PDC 点融合进当前边界
```

PDC（Parking Distance Control，泊车距离控制）使用超声波传感器数据，主要用于水平泊出场景的通道融合。

### 4.9 SDG/PDC 互斥关系

- 水平泊出模式 (`PARALLEL`): SDG 状态强制设为 `Keep`，全程采用 PDC 通道数据
- 非水平模式: SDG 正常工作，PDC 状态跟随 SDG 状态

---

## 5. Failcause 错误码汇总

| 码值 | 触发位置/条件 | 含义 |
|------|-------------|------|
| 1 | 建图过程 | 通用失败 |
| 2 | 建图过程 | 通用失败 |
| 45 | `CalMapInfo()` 返回 FALSE | 地图信息计算失败 |
| 47 | 边界点数不足 | 边界无效 |
| 48 | 车位信息缺失 | 车位参数获取失败 |
| 49 | 障碍物信息异常 | 障碍物数据错误 |
| 57 | 边界重建失败 | 边界融合失败 |
| 58 | 终点位置异常 | 终点超出范围 |
| 59 | 非首次建图帧左右边界点数 < 2 | 建图数据不足（非首次帧时边界有效性检查） |
| 100 | 车位边界计算失败 | FSD/OD 边界计算失败 |
| 101 | 多处(6x) | 共同边界融合过程失败 |

---

## 6. 编译条件开关

| 宏定义 | 功能 |
|--------|------|
| `SUPPORT_PARKING_OUT_SYSTEM` | **主开关**，控制整个模块编译 |
| `SUPPORT_PARKING_OUT_DEBUG` | 调试功能开关 |
| `SUPPORT_PARKING_OUT_UWB` | UWB（超宽带）定位支持 |
| `SUPPORT_BLIND_ALLEY_SLOT` | 死胡同车位场景 |
| `SUPPORT_ELECTRONIC_FENCE_MAP` | 电子围栏地图 |
| `APAMAP_PARKOUT_USE_SDG_OBJS` | 使用 SDG 感知障碍物 |
| `APAMAP_PARKOUT_FUS_SDG` | SDG 融合功能 |
| `APAMAP_USE_PDC_OBJS` | 使用 PDC 感知障碍物 |
| `APAMAP_PARKOUT_FUS_PDC` | PDC 融合功能 |
| `APAMAP_PARKOUT_USE_TOTALMAP_OBJS` | 使用全地图障碍物 |
| `APAMAP_PARKOUT_PCDEMO_USE_DEFAULT_SDG_OBJS` | PC 演示模式默认 SDG 障碍物 |
| `APA_MAP_PARKOUT_LADDER_SLOT` | 阶梯斜列车位类型 |
| `APA_MAP_PARK_OUT_WITH_VPLSLOTPTS_FROM_TOTALMAPINFO` | 从全景地图获取 VPL 车位点 |
| `APA_MAP_DEBUG_INFO_LIMITER` | 调试信息限制器 |
| `DEBUG_PRINT_SLOTOBJ` | 打印车位障碍物调试信息 |

---

## 7. 泊出模式支持

文件支持多种泊出模式，由 `parkoutmode` 参数控制：

| 模式 | 说明 | 边界生成策略 |
|------|------|------------|
| `PARALLEL` | 水平泊出 | 采用车位框 + PDC 通道融合 |
| `PERPENDICULAR` | 垂直泊出 | 采用车位框 + FSD/OD 融合 |
| `ANGLED` | 斜列泊出 | 支持阶梯斜列类型 + FSD/OD |
| `HEAD_TURN_ROUND` | 调头泊出 | 特殊终点判定逻辑 |

### 车位尺寸分类

| 分类 | 标志位 | 描述 |
|------|--------|------|
| 极小车位 | `bShortestSlotLen` | 水平极小车位 |
| 小车位 | `bShortSlotLen` | 水平小车位 |
| 极大车位 | `bLonggestSlotLen` | 水平极大车位 |

---

## 8. Git 提交历史

```
f93d7b5a fix()：更新版本4.10.53
98853a1c fix():【完全泊出】修改根据周围车位构造斜列车位 Gen2.P WI-9963
1ecdf0bd fix()：满足宽通道场景，采用历史的边界数据，防止主边界跳变频繁。（WI-9851）
df0f5e5d add:优化垂直车位类型车位框误修改为阶梯斜列类型，导致按斜列车位泊出建图的问题。（WI-9861）
f8e01762 水平完全泊出采用通道PDC融合边界
```

---

## 9. 潜在优化点

基于代码结构和功能分析，以下方面可能存在优化空间：

1. **边界融合多次叠加** — `CalMapInfo()` 中串行调用5种融合策略，每种都修改 `LeftBoundary/RightBoundary`，可能引入累积误差
2. **SDG/PDC 备份数据一致性** — `BkDataBfSDGFus` 和 `BkDataBfPDCFus` 独立管理，两者间无协调机制
3. **大量全局 BOOLEAN 标志位** — 27+ 个模块级标志位通过 `#ifdef` 条件管理状态机，可读性较差
4. **Failcause 码 101 出现 6 次** — 高频率失败码可能指向边界融合的薄弱环节
5. **终点位置动态更新** (`EndCarPosUpdata`) — 函数体~850 行，包含大量条件分支和几何计算，是运行时热点

---

*本文档由 Hermes Agent 自动生成，基于 git 恢复的源码（磁盘文件为 TSD 加密格式）*
