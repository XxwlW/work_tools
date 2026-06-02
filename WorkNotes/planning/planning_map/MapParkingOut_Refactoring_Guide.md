# MapParkingOut.cpp 源码分析与重构建议

> 分析日期：2026-05-11
> 源码位置：`src_readable/pnc_map/MapParkingOut.cpp`（13,863 行，~496KB）
> 对应项目：`/media/disk/2818_proj/perf_opt`
> 原始文本从 `dev_Loc` git 仓库提取（工作目录文件被编码为二进制格式，但 git 仓库保留了文本版本）

---

## 一、文件概况

MapParkingOut.cpp 是 **自动泊车（APA）系统中泊出场景的地图构建模块**，负责泊出过程中所有与地图相关的计算，包括车位构建、传感器数据融合、边界地图生成、终车位置计算等。

### 文件指标

| 指标 | 数值 |
|------|------|
| 总行数 | 13,863 |
| 函数总数 | ~55 个 |
| 全局变量 | 17 个 BOOLEAN + 3 个结构体 |
| 最大函数 | 1,584 行（ElectrFenceMapBulid） |
| 注释行 | ~609 行（含废弃代码注释） |
| 条件编译块 | ~30+ 处 |
| 错误码数量 | 11 种 |

---

## 二、代码架构总览

### 2.1 主任务入口

```
APAMap_ParkingOutTask()              [70行]
  │
  ├─ [首次调用/重算]
  │   APAMap_ParkingOutDebugInit()
  │   APAMap_ParkingOutCalMapSlotPar() ── 计算地图车位参数（基础配置）
  │   APAMap_ParkingOutCalSlotInfo()    ── 计算车位信息
  │   APAMap_ParkingOutCalMapInfo()     ── 计算地图信息（核心融合）
  │   APAMap_ParkingOutCheckIfCarPosIsValid() ── 检查车辆位置有效性
  │
  └─ [非首次/更新]
      APAMap_ParkingOutUpDataMapInfo()  ── 更新地图输出信息
```

### 2.2 功能模块划分

#### 模块A：主任务调度（31-99行，1个函数）
- `APAMap_ParkingOutTask()` - 顶层任务调度

#### 模块B：地图参数和车位计算（101-262行，2个函数）
- `APAMap_ParkingOutCalMapSlotPar()` - 计算地图车位参数（142行）
- `APAMap_ParkingOutCalSlotInfo()` - 计算车位信息（仅为入口，20行）

#### 模块C：传感器数据筛选排序（11953-12520行，2个函数）
- `APAMap_ParkingOutSiftAndSeqSDGPts()` - SDG全景点筛选排序（372行）
- `APAMap_ParkingOutGetSDGInfoByParkMode()` - 按泊出模式获取SDG信息（184行）

#### 模块D：SDG/PDC 数据获取与融合（12521-13800行，12个函数）
- `APAMap_ParkingOutGetBkSDGOutPutData()` / `SaveBkSDGOutPutData()` - 备份SDG输出
- `APAMap_ParkingOutSetMainSlotBordInfoByBkDataBfSDGFus()` - 从备份恢复主边界
- `APAMap_ParkingOutDeleteMainSlotBord()` - 删除主车位边界（100行）
- `APAMap_ParkingOutSaveBkDataBfSDGFus()` - SDG融合前保存备份
- `APAMap_ParkingOutUpDataMapBoundaryBySDGInfo()` - 用SDG更新地图边界
- `APAMap_ParkingOutCheckIfFusBoundarySDGInfo()` - 检查SDG融合结果（142行）
- `APAMap_ParkingOutGetSDGInfoPt()` - 获取SDG信息点（100行）
- `APAMap_ParkingOutFusBoundaryBySDGInfo()` - SDG融合边界（**762行**）
- `APAMap_ParkingOutGetEightParkOutMode()` - 获取8种泊出模式（4行）

#### 模块E：边界融合（274-4479行，6个函数）
- `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()` - FSD+OD计算边界（**671行**）
- `APAMap_ParkingOutCalSlotBorderPtByParkOutInfo()` - 泊出信息计算边界（237行）
- `APAMap_CalSlotBorderPtOffsetBySensorMapInfo()` - 传感器偏移计算
- `APAMAP_GetSlotBdPtBySensorObjs()` - 获取传感器车位边界点（194行）
- `APAMap_ParkingOutFusBoundaryByFSDMapInfo()` - FSD融合边界（**658行**）
- `APAMap_ParkingOutFusBoundaryByLaneLineMapInfo()` - 车道线融合边界（**615行**）
- `APAMap_ParkingOutFusBoundaryByRefercLineMapInfo()` - 参考线融合边界（**583行 + 611行**，两个重载）

#### 模块F：终车位置计算（4595-6299行，6个函数）
- `APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB()` - UWB下设置（323行）
- `APAMap_ParkingOutSetEndCarPosInOldCorSys()` - 常规设置（244行）
- `APAMap_ParkingOutCenterEndCarPosInfo()` - 居中计算（246行）
- `APAMap_ParkingOutBoundarySeizeEndCarPosInfo()` - 边界捕获（**724行**）
- `APAMap_ParkingOutEndCarPosUpdata()` - 更新终车位置（167行）

#### 模块G：OD障碍物处理（6300-6935行，3个函数）
- `APAMAP_ParkingOutGetSlotBdPtByODObjs()` - OD获取边界点（305行）
- `APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo()` - OD偏移计算
- `APAMap_ParkingOutPickDispersedObstacles()` - 拾取离散障碍物（321行）

#### 模块H：场景模式与电子围栏（6936-9230行，4个函数）
- `APAMap_ParkingOutUpDataMapInfoBySlotCorInfo()` - 车位校正更新
- `APAMap_ParkingOutObliqueRowStairsInfo()` - 斜列车位台阶信息（170行）
- `APAMap_ParkingOutMapScenarioModeCheck()` - 场景模式检查（**484行**）
- `APAMap_ParkingOutElectrFenceMapBulid()` - 电子围栏构建（**1584行**）

#### 模块I：车位构建与VPL处理（9230-11952行，8个函数）
- `APAMap_ParkingOutReOrderVPLSlotPtsByParkOutMode()` - 重排VPL点（93行）
- `APAMap_ParkingOutCheckIfTargetSlotIsLadderSlot()` - 判断梯子车位（204行）
- `APAMap_ParkingOutGetSlotInfoFromVPLSlotPts()` - 从VPL提取信息（252行）
- `APAMap_ParkingOutBuildSlotByOneSideNearbySlot()` - 单侧邻近建房（197行）
- `APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot()` - 当前车位单侧（253行）
- `APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot()` - 当前车位双侧（239行）
- `APAMap_ParkingOutBuildSlotByTwoNearbySlot()` - 双侧邻近建房（265行）
- `APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap()` - VPL全局地图参数（**1015行**）

#### 模块J：辅助工具与线参数（785-977行，3个函数）
- `AlgCom_SetParkOutObj1Pt()` - 设置Obj1点（67行）
- `AlgCom_SetParkOutObj2Pt()` - 设置Obj2点（67行）
- `AlgCom_SetParkingOutObjAng()` - 设置目标角度（26行）
- `APAMap_ParkingOutLineParABCByMainSlotBord()` - 主边界线参数（35行）
- `APAMap_ParkingOutLineParABCbyPoints()` - 点拟合线参数（33行）

---

## 三、关键数据流

### 3.1 输入数据

```
APAMap_GInputData (全局变量)
  ├── ParkReqPar.Request_cmd         - 请求命令（1=泊出, 6=重算）
  ├── ParkReqPar.parkoutmode          - 泊出模式（平行/垂直/斜列）
  ├── ParkReqPar.parkside             - 泊出侧（左/右）
  ├── ParkReqPar.Request_SlotId       - 车位ID
  ├── ParkReqPar.APARunningstate      - APA运行状态
  └── ParkReqPar.APAstate             - APA状态
```

### 3.2 传感器数据

| 传感器 | 功能 | 处理函数 |
|--------|------|----------|
| VPL（视觉车位线） | 获取车位线点、角度、深度 | `GetSlotInfoFromVPLSlotPts`, `CalSlotParByVPLSlotInfoFromTotalMap` |
| SDG（全景影像） | 获取左右两侧障碍物边界 | `SiftAndSeqSDGPts`, `FusBoundaryBySDGInfo` |
| PDC（超声波雷达） | 获取前后方障碍物边界 | `GetSDGInfoByParkMode`(注意命名其实是PDC相关) |
| FSD（前视/环视） | 获取前方自由空间边界 | `FusBoundaryByFSDMapInfo` |
| OD（障碍物检测） | 离散障碍物 | `PickDispersedObstacles`, `GetSlotBdPtByODObjs` |
| LaneLine（车道线） | 车道线辅助定位 | `FusBoundaryByLaneLineMapInfo` |
| RefercLine（参考线） | 参考线辅助定位 | `FusBoundaryByRefercLineMapInfo` |

### 3.3 传感器融合 Pipeline

```
VPL数据 ──→ 车位参数 ──→ 基础车位框
                               │
SDG数据 ──────────────┐        ▼
PDC数据 ──────────────┤──→ 边界融合 ──→ 电子围栏地图
FSD数据 ──────────────┤          │
OD数据  ──────────────┘          │
                                 ▼
LaneLine ──────────────┐   终车位置计算
RefercLine ────────────┤         │
                        │        ▼
                        └──→ 最终地图输出
```

---

## 四、代码质量问题

### 问题1：超大函数（Monster Functions）

| 函数 | 行数 | 风险 |
|------|------|------|
| `ElectrFenceMapBulid` | 1,584 | 单一函数处理所有电子围栏逻辑，无法单独测试 |
| `CalSlotParByVPLSlotInfoFromTotalMap` | 1,015 | VPL参数计算混合了多种场景判断 |
| `FusBoundaryBySDGInfo` | 762 | SDG融合逻辑过长 |
| `BoundarySeizeEndCarPosInfo` | 724 | 边界捕获逻辑过长 |
| `CalSlotBorderPtByFSDAndODMapInfo` | 671 | FSD+OD计算混合 |
| `FusBoundaryByFSDMapInfo` | 658 | FSD融合 |
| 合计 6 个函数超过 500 行 | | **占总代码量 ~43%** |

### 问题2：17 个文件作用域全局标志位

```
BOOLEAN bCntAddFlag                    - cnt+1轨迹重算标志
BOOLEAN bLaneLineUpdateEndCarPosFlag   - 车道线更新终点位置
BOOLEAN bRefercLineUpdateEndCarPosFlag - 参考线更新终点位置
BOOLEAN bAfterNewAnchorPointFlag       - 锚点转换后标志
BOOLEAN bFsdInRightOfEndCarPosFlag     - FSD入侵右边标志
BOOLEAN bFsdFromMapMainSlotBordFlag    - 入侵来自主边界
BOOLEAN bFsdFromMapSubSlotBordFlag     - 入侵来自子边界
BOOLEAN bFsdFromMapMainAndSubSlotBordFlag - 入侵来自主子边界
BOOLEAN bPreventStepNRedundantFlag     - 防多走
BOOLEAN bShortestSlotLen               - 极小车位
BOOLEAN bShortSlotLen                  - 小车位
BOOLEAN bLonggestSlotLen               - 极大车位
BOOLEAN bCarryOutSlot                  - 采用车位框
BOOLEAN bLabelAngledFlag               - 斜列车位框
BOOLEAN bObjLabelLadderFlag            - 斜列阶梯车位框
BOOLEAN bLabelAngledParkingOutSlotFlag - 斜列车位泊出后
BOOLEAN bODWheelChockFlag              - 水平泊出车位内有轮挡
```

**问题**：这些标志位在多个函数之间共享状态，修改一个函数可能影响其他函数的逻辑，极难追踪。

### 问题3：冗余重复代码

**示例1**：`APAMap_ParkingOutCalMapSlotPar()` 和 `APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo()` 中有大段几乎相同的逻辑：
- 同样是判断 `SlotLength < APAMap_ComCfg.APASlotMinSmallSlotLen - 150.0`
- 同样是计算 `TempPt1.x = ±HalfWidthOfCar` 等
- 区别仅在于上下文不同

**示例2**：`APAMap_ParkingOutSetEndCarPosInOldCorSys()` 和 `APAMap_ParkingOutSetEndCarPosInOldCorSysByUWB()` 有大量重复逻辑。

### 问题4：深层嵌套

```c
// 从 CalMapSlotPar（142行函数）中提取的一段：
if (ParkOutMode == PARALLEL)
  if (ParkSide == LEFT)
    if (SlotDataIsNotMirrored == TRUE)
      bSlotDataAtRigthSide = TRUE;
    else
      bSlotDataAtRigthSide = FALSE;
  else
    if (SlotDataIsNotMirrored == TRUE)
      bSlotDataAtRigthSide = FALSE;
    else
      bSlotDataAtRigthSide = TRUE;
else
  if (ParkSide == LEFT)
    ...
```

这种 4 层嵌套的 if-else 逻辑可以用一个 2D lookup table 代替。

### 问题5：魔法数字

```c
// 散布在各处的硬编码数值
OffsetX + 300           // 偏移补偿
LenBetweenRAxisAndFBumper + 1500  // 前向探测距离
TempDis > -3500         // PDC融合判断阈值（原为-1500，手动改到-3500）
SlotLength < 4500       // 极小车位
SlotDepth < 2500        // 浅车位
```

### 问题6：条件编译导致的多个变体

约 30+ 处 `#ifdef`，使同一份代码同时存在多个编译路径，环境切换时测试覆盖可能遗漏：

```c
#ifdef SUPPORT_PARKING_OUT_UWB
    // UWB 逻辑
#else
    // 非 UWB 逻辑
#endif

#ifdef APAMAP_PARKOUT_USE_TOTALMAP_OBJS
    // 全局地图障碍物逻辑
#else
    // 本地障碍物逻辑
#endif
```

### 问题7：命名不统一

- 中英文混写注释：`//cnt+1轨迹重算标志位，下一帧会清空FALSE`
- 拼写错误：`SlotBord` 而非 `SlotBorder`，`Updata` 而非 `Update`，`Bulid` 而非 `Build`
- `APAMap_ParkingOutElectrFenceMapBulid` 中 `Electr` 而非 `Electric`

---

## 五、重构建议

### Phase 1：立即可做（无需修改业务逻辑）

| 任务 | 说明 | 工作量 |
|------|------|--------|
| **1.1 提取文本源码** | 从 `dev_Loc` git 仓库导出完整文本版本 | 已完成 |
| **1.2 建立函数调用图** | 使用 Doxygen + Graphviz 生成调用关系图 | 半天 |
| **1.3 补充函数头注释** | 每个函数加 @brief @param @return | 1天 |
| **1.4 替换魔法数字为常量** | `300` → `CAR_OFFSET_COMPENSATION` 等 | 1天 |
| **1.5 抽真值表** | 用二维数组替换 4 层 if-else 嵌套（如 bSlotDataAtRigthSide 判断） | 0.5天 |
| **1.6 修拼写错误** | `Bord`→`Border`, `Updata`→`Update`, `Bulid`→`Build` | 0.5天 |

### Phase 2：中危重构（需要回归测试）

| 任务 | 说明 | 工作量 |
|------|------|--------|
| **2.1 全局变量收拢为结构体** | 17个 BOOLEAN → `ParkingOutState` 结构体 | 1天 |
| **2.2 拆分超大函数** | `ElectrFenceMapBulid(1584行)` → 8-10个小函数 | 3天 |
| **2.3 拆分 `CalSlotParByVPLSlotInfoFromTotalMap(1015行)`** | 按车位类型（平行/垂直/斜列）分离 | 2天 |
| **2.4 去重通用逻辑** | `CalMapSlotPar` 和 `CalSlotBorderPtByParkOutSlotInfo` 公用函数提取 | 1天 |
| **2.5 UWB/Non-UWB 统一** | 终端函数内的 ifdef → 策略模式 | 2天 |
| **2.6 终车位置函数简化** | `SetEndCarPosInOldCorSys` + `SetEndCarPosInOldCorSysByUWB` 合并 | 1天 |

### Phase 3：长期重构（架构改进）

| 任务 | 说明 | 工作量 |
|------|------|--------|
| **3.1 类封装** | C风格函数 → `MapParkingOut` 类，成员函数 + 成员变量 | 3天 |
| **3.2 Pipeline 模式** | 传感器数据 → 车位构建 → 边界融合 → 终车位置 各阶段独立 | 2天 |
| **3.3 策略模式替代条件编译** | `#ifdef SUPPORT_PARKING_OUT_UWB` 等 → 运行时策略选择 | 3天 |
| **3.4 枚举化魔法参数** | `uint8_t_INF ParkOutMode` → `enum class ParkOutMode` | 1天 |
| **3.5 单元测试框架** | 为每个处理阶段添加独立测试用例 | 5天 |

### 重构优先级矩阵

```
高价值 + 低风险（立刻做）
  ├── 替换魔法数字为常量
  ├── 抽真值表（4层if-else → lookup table）
  └── 修拼写错误

高价值 + 中等风险（本周做）
  ├── 全局变量收拢
  ├── 拆分 ElectrFenceMapBulid
  └── 拆分 CalSlotParByVPLSlotInfoFromTotalMap

高价值 + 高风险（规划中）
  ├── 类封装
  ├── Pipeline 模式
  └── 单元测试

低价值（不着急）
  ├── 条件编译转策略模式
  └── Doxygen 调用图
```

---

## 六、代码复原文案

原始二进制文件已经被编码，但 git 仓库保留了文本版本。

**恢复方法**：
```bash
cd /media/disk/2818_proj/dev_Loc
git show HEAD:src/pnc_map/MapParkingOut.cpp > /path/to/recover/MapParkingOut.cpp
git show HEAD:src/pnc_map/MapParkingOut.h > /path/to/recover/MapParkingOut.h
```

已恢复的文件放在：
- `/media/disk/2818_proj/perf_opt/src_readable/pnc_map/MapParkingOut.cpp`（13,863 行）
- `/media/disk/2818_proj/perf_opt/src_readable/pnc_map/MapParkingOut.h`（207 行）

---

## 七、相关文档

- `/media/disk/2818_proj/perf_opt/doc/PlanningManual_x86.docx` - Planning 手册
- `/media/disk/2818_proj/perf_opt/doc/algorithm_specs/` - 算法方案说明
- `Doxyfile.doxygen` - 项目 Doxygen 配置文件（已有，可直接生成文档）

---

## 八、总结

MapParkingOut.cpp 是一个功能完整的嵌入式泊车地图模块，核心逻辑清晰但编码质量属于典型的**嵌入式 C 风格**：

- **优点**：逻辑完整、经过了大量实车测试验证、注释使用了中文便于团队理解
- **缺点**：全局状态过多、函数过于庞大、存在重复代码、魔法数字散布

**推荐策略**：先做 Phase 1 和 Phase 2 中的高价值低风险项，确保不影响现有功能的前提下逐步改善可维护性。最重要的是**建立回归测试**，确保重构不会引入实车 bug。
