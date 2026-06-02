# 车位边界点融合相关函数分析

> **源文件**：`src/pnc_map/MapParkingOut.cpp`（TSD-Header 编码，已解码）
> **分析日期**：2026/06/01
> **涉及函数**：
> 1. `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo` — 第 274-783 行（510 行）
> 2. `APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo` — 第 6882-6891 行（10 行，wrapper）
> 3. `APAMAP_ParkingOutGetSlotBdPtByODObjs` — 第 6573-6881 行（309 行）

---

## 一、三个函数的关系总览

```
┌─────────────────────────────────────────────────────────────┐
│ 调用方 (ParkOut 主流程)                                      │
│   APAMap_ParkingOutTask()                                    │
│   └─ APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo()    │  ← 函数①
└─────────────────────────────┬───────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
  ┌─────────────┐     ┌──────────────┐     ┌────────────────┐
  │ CalSlotBord │     │ CalSlotBord  │     │ CalSlotBord    │
  │ PtOffsetBy  │     │ PtOffsetBy   │     │ PtOffsetBy     │
  │ SensorMap   │     │ TopViewFSD   │     │ ODMapInfo      │ ← 函数②
  │ Info        │     │ MapInfo      │     │ (wrapper)      │
  └─────────────┘     └──────────────┘     └───────┬────────┘
                                                    │
                                                    ▼
                                          ┌──────────────────┐
                                          │ GetSlotBdPtBy    │
                                          │ ODObjs(0, ...)   │ ← 函数③
                                          │ GetSlotBdPtBy    │
                                          │ ODObjs(1, ...)   │
                                          └──────────────────┘
```

**调用链**：
- **函数①** = 主融合函数，融合 3 类数据源（Sensors + FSD + OD）后更新车位边界点和终点
- **函数②** = 函数①的子调用，负责"OD 数据偏移"部分（只是函数③的两次调用封装）
- **函数③** = 实际遍历 OD 障碍物并计算 OffsetX/OffsetY 的算法函数

---

## 二、函数① `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo`

### 2.1 函数签名

```c
BOOLEAN APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo(void);
```

无入参（所有数据来自全局），返回 `BOOLEAN`：
- `TRUE`：成功，更新了车位边界和终点
- `FALSE`：车位净距不足，`APAMAP_Setfailcause(58)` 后退出

### 2.2 核心职责

> 把 **传感器** / **FSD** / **OD 视觉** 三类数据源对车位边界的"入侵"信息融合，**修正车位边界点 Obj1Pt/Obj2Pt**，再调用 `APAMap_ParkingOutSetEndCarPosInOldCorSys` 更新终点位置。

### 2.3 整体流程图

```
┌──────────────────────────────────────────────────────────────┐
│ ① 入口参数装载 (316-332)                                      │
│   拷贝 Obj2Pt/Obj1Pt/SlotLen/EndPos/EndPosLine/OrgAng/OrgPt  │
│   bSlotDataAtRigthSide, ParkOutMode, CurCarCoordinateX       │
│   初始化 SafeDis=250, BloundaryOffsetY=0, bSeizeEndCarPosFlag=FALSE │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ② 取三类偏移量 (334-404)                                      │
│   ┌─ Sensor: APAMap_CalSlotBorderPtOffsetBySensorMapInfo    │
│   ├─ FSD   : APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo│
│   └─ OD    : APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo│
│                                                              │
│   → 融合策略: 各类取最大值 (max) 作为最终 OffsetX1/2/Y1/Y2   │
│   → OD 额外过滤: < 50 视为无入侵                              │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ③ 计算车位内净距 fDis1/fDis2 (406-421)                        │
│   - bCarryOutSlot=T → 用 APAMap_GetSearchMaxInnerY (从边界外)│
│   - 否则            → 用 AlgCom_GetPointToLineDis (点到边线)│
│                      再减车身投影 TempDis1                    │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ④ 默认边界更新标志 bUpdataDefaulBordenFlag (423-460)          │
│   - 仅 bAfterNewAnchorPointFlag=T 时按 ParkOutMode 判定:     │
│     · HEAD_TURN_ROUND  : CurCarCoordinateX > -1              │
│     · REAR_TURN_ROUND  : CurCarCoordinateX > 2               │
│     · 其它             : CurCarCoordinateX > 0               │
│   - 斜列车位 (bLabelAngledFlag=T) 额外考虑                    │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ⑤ DefaultOffsetY 兜底 (461-478)                              │
│   - 非 PARALLEL 且不采用车位框时: DefaultOffsetY = fDis-600   │
│   - 若 OffsetY < DefaultOffsetY 则取 DefaultOffsetY          │
│   - 若 bUpdataDefaulBordenFlag=T: OffsetX/Y 全部清零          │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ⑥ 车位净距安全检查 (487-523)                                  │
│   NewDis = fDis1 + fDis2 - OffsetY1 - OffsetY2               │
│   if (NewDis < 2 * 250 = 500): 报 58 退出 (非 USSLOT)        │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ⑦ 极小车位保护 (525-535)                                     │
│   - PARALLEL: SlotLen < 车长+700 → OffsetY = 0              │
│   - 其它    : SlotLen < 车宽+500 → OffsetY = 0              │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ⑧ 修正 Obj1Pt (537-566)                                      │
│   TempLine2 ← Obj1Ang 方向的轴线, 过 Obj1Pt                  │
│   if OffsetY1: 平移平行线 (短车位再减 100)                     │
│   TempLine1 ← OrgAng 方向的轴线, 过 Obj1Pt                  │
│   if OffsetX1: 平移平行线                                    │
│   if 任一非零: 求两线交点 → Obj1Pt                             │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ⑨ 修正 Obj2Pt (568-623)                                      │
│   类似 Obj1, 额外条件:                                       │
│   - bCntAddFlag + 车辆已驶出 → OffsetX2 = 0                  │
│   - PARALLEL + 短车位 + 锚点前 → OffsetX2 = 0                │
│   - bShortSlotLen=T 时, OffsetX2 强制走平移分支              │
│   - 极小车位 (bShortestSlotLen): TempDis = OffsetY2-150      │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ⑩ CarPosInvadeSlotBorderInfo 检查 (624)                       │
│   检查保险杠是否在边界内, 是则向外推开边界                    │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ⑪ 重算 SlotLen (625-628)                                     │
│   用修正后 Obj1Pt 到 Obj2 轴线距离                            │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ⑫ 车位长分级 (630-655)                                       │
│   PARALLEL:                                                    │
│     > 车长+2000 → bLonggestSlotLen=T                          │
│     ≤ 车长+1500 → bShortSlotLen=T                            │
│     ≤ 车长+1100 → bShortestSlotLen=T                         │
│   其它: 清 bShortSlotLen/bShortestSlotLen                     │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ⑬ 终点重算 EndPos (657-704)                                   │
│   条件: 锚点已转换/车道线/参考线 → 沿用旧 EndPos             │
│   否则: 调用 APAMap_ParkingOutSetEndCarPosInOldCorSys         │
│         (UWB 模式下优先用 UWB 版本)                            │
│         EndPosLine = 过 EndPos 的车轴线                       │
│   异常 (0xff): Setfailcause(101) + return FALSE              │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ⑭ 写回全局 (732-781)                                          │
│   bObj1/2Exist (若任一非零)                                   │
│   SlotPar.Obj2Pt = Obj2Pt; Obj1Pt = Obj1Pt                    │
│   SlotBordPt[0/1] = Obj2Pt/Obj1Pt                             │
│   if OffsetX2 != 0: SlotBordPt[0] = Obj2Ang轴线 ∩ OrgAng轴线  │
│   if OffsetX1 != 0: SlotBordPt[1] = Obj1Ang轴线 ∩ Obj2Ang轴线│
│   NewCordSysOPt = SlotBordPt[0]   ← 重置坐标系原点           │
│   SlotPar.SlotLen, EndPos, EndPosLine                         │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
                      return TRUE
```

### 2.4 关键变量对照表

| 原名 | 重构版 | 含义 |
|------|--------|------|
| `Obj2Pt` / `Obj1Pt` | `border_point_obj2` / `border_point_obj1` | 车位外/内侧边界点（输出） |
| `SlotLen` | `slot_length` | 修正后车位长 |
| `fDis1` / `fDis2` | `inner_distance_obj1` / `inner_distance_obj2` | 边界到对面轴线的内净距 |
| `FSDOffset*` | `fsd_invasion_offset_*` | 顶视 FSD 入侵量 |
| `SensorOffset*` | `sensor_invasion_offset_*` | 超声波入侵量 |
| `ODOffset*` | `od_invasion_offset_*` | 视觉 OD 入侵量 |
| `OffsetX1/Y1` | `merged_offset_obj1_x/y` | Obj1 融合后最终偏移 |
| `OffsetX2/Y2` | `merged_offset_obj2_x/y` | Obj2 融合后最终偏移 |
| `DefaultOffsetY1/Y2` | `default_offset_y_*` | 兜底偏移（fDis - 600） |
| `NewDis1/NewDis2/NewDis` | `remaining_distance_*` | 减去偏移后剩余净距 |
| `bUpdataDefaulBordenFlag` | `should_use_default_border` | 走默认边界（绕开 OD/FSD） |
| `bSeizeEndCarPosFlag` | `is_fsd_invading_end_pos` | FSD 是否侵占终点 |
| `BloundaryOffsetY` | `boundary_offset_y` | 极小车位专属 Y 补偿 |
| `bLonggestSlotLen` | `is_oversized_slot` | 车位过长 |
| `bShortSlotLen` | `is_short_slot` | 车位偏短 |
| `bShortestSlotLen` | `is_shortest_slot` | 车位极短（Obj2 偏移再减 150） |

### 2.5 数据融合策略（三类数据源）

```
                    ┌──────────────┐
                    │ FSD 顶视     │
                    │ FSDOffsetX/Y │
                    └──────┬───────┘
                           │ max
                    ┌──────┴───────┐
                    │ Sensor 超声  │
                    │ SensorOffset │
                    └──────┬───────┘
                           │ max
                    ┌──────┴───────┐
                    │ OD 视觉      │
                    │ ODOffset     │
                    │ (< 50 视为0) │
                    └──────┬───────┘
                           │ max
                    ┌──────┴───────┐
                    │ DefaultOffset│
                    │ = fDis - 600 │  (非PARALLEL非车位框)
                    └──────┬───────┘
                           │ max
                    ┌──────┴───────┐
                    │ 最终 Offset  │
                    │ OffsetX1/Y1  │
                    │ OffsetX2/Y2  │
                    └──────────────┘
```

> **融合原则** = **取最大值**（保守策略：哪边报警最近就以哪边为准）。
> 只有当车已驶出车位（`bUpdataDefaulBordenFlag=T`）时，所有 Offset 强制清零，**不再避障**。

### 2.6 边界修正几何（步骤 ⑧⑨）

**问题**：已知 Obj1/2 的偏移量 (OffsetX, OffsetY)，如何反算新的边界点？

**方法** = **平移相交法**：

```
                ObjAng 方向
                    │
  TempLine2 ─ ─ ─ ─●─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   (Obj1 方向的边界线)
                    │ ← TempDis = OffsetY
                    │
  TempLine2' ─ ─ ─ ●─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   (OffsetY 平移后)
                    │
                    │  OrgAng 方向
                    │
  TempLine1 ─ ─ ─ ─●─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   (车位长边方向)
                    │ ← TempDis = OffsetX
                    │
  TempLine1' ─ ─ ─ ●─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   (OffsetX 平移后)
                    │
                    ▼
                   交点 = 新 Obj1Pt / Obj2Pt
```

**关键函数**：
- `AlgCom_LineParABCByCurrentCarPosition(&TempCarPos, 0)` → 由点+方向生成 Ax+By+C=0 直线
- `AlgCom_LineParABCByLineAngAndDisBtwGivenParalLine(line, ang, dis)` → 平移 dis 距离
- `AlgCom_CrossPointOfTwoLines(line1, line2, &pt)` → 求交点

### 2.7 短车位/极小车位的特殊处理

```
SlotLen 阈值 (mm):
                         1100  +车长  1500  +车长   2000  +车长
                          │       │       │       │       │
PARALLEL 模式:             │       │       │       │       │
  < 车长+700  → OffsetY=0 (不避障)
  ≤ 车长+1100 → bShortestSlotLen=T (Obj2 偏移再减 150)
  ≤ 车长+1500 → bShortSlotLen=T
  > 车长+2000 → bLonggestSlotLen=T

其它模式:
  < 车宽+500  → OffsetY=0
```

> 短车位里"剩余空间不够"，强行避障反而让车出不去。

### 2.8 终点重算决策（步骤 ⑬）

```
                   ┌─ 锚点已转换 ────┐
                   ├─ 车道线更新 ────┼─→ 沿用旧 EndPos (不再算)
                   └─ 参考线更新 ────┘
                          │ 否
                          ▼
                ┌──────────────────────┐
                │ UWB 模式开启?        │
                │ 且 UWB 数据有效?    │
                └────┬────────┬───────┘
                  yes │        │ no
                      ▼        ▼
              SetEndByUWB   SetEndByFSDAndOD
                      │        │
                      └────┬───┘
                           ▼
                    检查 0xff 异常
                           │
                       异常 → 报 101
```

---

## 三、函数② `APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo`

### 3.1 函数签名

```c
void APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo(
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX2,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2);
```

### 3.2 实现（仅 10 行）

```c
void APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo(
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX2,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2) {
  APAMAP_ParkingOutGetSlotBdPtByODObjs(0, pOffsetX1, pOffsetY1);
  APAMAP_ParkingOutGetSlotBdPtByODObjs(1, pOffsetX2, pOffsetY2);
  return;
}
```

### 3.3 作用

> **纯包装器**。把函数③对 Obj1/Obj2 各调用一次，统一输出 4 个偏移量。

| 边界 | 入参 | 调用 | 输出 |
|------|------|------|------|
| Obj1 | `Bordpttype=0` | `APAMAP_ParkingOutGetSlotBdPtByODObjs(0, ...)` | `pOffsetX1, pOffsetY1` |
| Obj2 | `Bordpttype=1` | `APAMAP_ParkingOutGetSlotBdPtByODObjs(1, ...)` | `pOffsetX2, pOffsetY2` |

> 注：参数名 `pOffsetX1/Y1` 对应 Obj1，`pOffsetX2/Y2` 对应 Obj2。下标和边界编号一致，方便在函数①中用 `1`/`2` 后缀统一处理。

---

## 四、函数③ `APAMAP_ParkingOutGetSlotBdPtByODObjs`

### 4.1 函数签名

```c
void APAMAP_ParkingOutGetSlotBdPtByODObjs(
    APA_ENUM_TYPE Bordpttype,                       // 0=Obj1, 1=Obj2
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX,          // 输出 X 偏移
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY);         // 输出 Y 偏移
```

### 4.2 核心职责

> 遍历 OD 视觉感知到的车位**外部障碍物**（Square 类的 WarningPost/ConeBucket/... + Polygon 类的 Curb 路边石），对每个障碍物调用 `APAMAP_GetSlotBdPtOffsetByGivenObjPts` 计算它对车位边界的入侵量，**取所有障碍物的最大 OffsetX/Y 作为最终输出**。

### 4.3 整体流程图

```
┌──────────────────────────────────────────────────────────────┐
│ ① 入口检查 (6611-6625)                                        │
│   ODInfo.TimeStamp == 0 → 输出 0,0, return                    │
│   取 ODInfo: TotalMapInfo.mapData.ODInfo                       │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ② 初始化参数 (6626-6690)                                      │
│   - ParkOutMode, MaxOutOffsetX=2000, MaxOutOffsetY=1000       │
│   - MaxInnerOffsetX: PARALLEL=1500, 其它=2800                  │
│   - bDataAtRightSide, OrgAng, OrgPt                           │
│   - ObjAng: Bordpttype=0→Obj1Ang, 1→Obj2Ang                   │
│   - TargetXLoc / TargetYLoc  ← 由 bDataAtRightSide 决定        │
│   - 构造 LineY (OrgAng 方向射线)                               │
│   - 构造 LineX (ObjAng 方向射线, 起点 = Obj1Pt 或 Obj2Pt)      │
│   - MaxInnerOffsetY = SearchMaxInnerY - 300, 下限 300          │
│   - OffsetX = -MaxInnerOffsetX (负值起步)                       │
│   - OffsetY = -MaxOutOffsetY                                    │
│   - ODInSlotPtForOffsetX/Y = NO_OBJ_DISTANCE                   │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ③ 双层 while 循环遍历障碍物 (6691-6858)                       │
│   外层: j = 0 → 1 → 2 (状态机: Square → Polygon → Done)        │
│   内层: i = 0..N                                                │
│                                                              │
│   j=0: 遍历 Square.Quadrilaterals, 选 Label 是:                 │
│        WarningPost, ConeBucket, SquareColumn,                  │
│        TwoWheelsVehicle, NoParkingSign, UPILLAR, Stone_Piers   │
│   j=1: 遍历 Polygon.Polygons, 选 Label = Curb (路边石)         │
│   j=2: bSearch = FALSE 退出                                    │
│                                                              │
│   每次找到一个障碍物: 拷 4/多边形点到 Data[], 调                │
│   APAMAP_GetSlotBdPtOffsetByGivenObjPts 算 OffsetX/Y            │
│   取**最大** (PreOffsetX < OffsetX 才更新)                      │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ ④ 过滤并输出 (6867-6880)                                      │
│   if (OffsetX > 50 || OffsetY > 50):                           │
│       if OffsetX < 0: OffsetX = 0   (负值置 0)                 │
│       *pOffsetX = OffsetX                                       │
│       *pOffsetY = OffsetY                                       │
│   else:                                                        │
│       *pOffsetX = 0                                             │
│       *pOffsetY = 0                                             │
│   APAMap_GInfo.SlotPar.ODPt[Bordpttype] = ODInSlotPtForOffsetX │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
                      return
```

### 4.4 关键变量对照表

| 原名 | 重构版 | 含义 |
|------|--------|------|
| `Bordpttype` | `border_type` | 0=Obj1, 1=Obj2 |
| `pODInfo` | `od_info` | 障碍物数据源 |
| `CurObjComInfo` | `current_obj_info` | 当前障碍物元信息 |
| `OrgAng` / `OrgPt` | `anchor_angle` / `anchor_origin` | 车位坐标系基准 |
| `ObjAng` | `target_boundary_angle` | Obj1/2 的方向角 |
| `TargetXLoc` / `TargetYLoc` | `search_target_x/y_loc` | 搜索方向（左/右） |
| `LineXStrPt` / `LineXEndPt` | `axis_x_start_pt` / `axis_x_end_pt` | X 方向搜索线起止 |
| `LineYStrPt` / `LineYEndPt` | `axis_y_start_pt` / `axis_y_end_pt` | Y 方向搜索线起止 |
| `Data[10]` | `obstacle_pts[10]` | 当前障碍物轮廓点（最多 10） |
| `DataNum` | `obstacle_pts_count` | 障碍物点数 |
| `MaxOutOffsetX` | `max_outward_offset_x` | 向外最大 X 偏移 (2000) |
| `MaxInnerOffsetX` | `max_inward_offset_x` | 向内最大 X 偏移 (PARALLEL=1500, 其它=2800) |
| `MaxOutOffsetY` | `max_outward_offset_y` | 向外最大 Y 偏移 (1000) |
| `MaxInnerOffsetY` | `max_inward_offset_y` | 向内最大 Y 偏移 (SearchMax-300) |
| `OffsetX` / `OffsetY` | `current_offset_x` / `current_offset_y` | 累积最大偏移 |
| `PreOffsetX` / `PreOffsetY` | `previous_offset_x` / `previous_offset_y` | 上一轮的偏移 |
| `ODInSlotPtForOffsetX` | `od_point_for_offset_x` | 触发最大 X 偏移的"罪魁祸首"点 |
| `ODInSlotPtForOffsetY` | `od_point_for_offset_y` | 触发最大 Y 偏移的"罪魁祸首"点 |

### 4.5 搜索方向决策表（`TargetXLoc` / `TargetYLoc`）

| 车位位置 | `Bordpttype` | `TargetYLoc` | `TargetXLoc` |
|---------|-------------|-------------|-------------|
| 右侧 (bDataAtRightSide=T) | 0 (Obj1) | 1 (right) | 0 (left) |
| 右侧 | 1 (Obj2) | 0 (left) | 0 (left) |
| 左侧 (bDataAtRightSide=F) | 0 (Obj1) | 0 (left) | 1 (right) |
| 左侧 | 1 (Obj2) | 1 (right) | 1 (right) |

> 这个决策决定了 `APAMAP_GetSlotBdPtOffsetByGivenObjPts` 沿哪个方向搜索障碍物到搜索线的距离。

### 4.6 状态机 `j` 的遍历流程

```
j=0  (Square 方形物体)
  ├─ i=0..Square.ObjNum
  ├─ 找 Label ∈ {WarningPost, ConeBucket, SquareColumn,
  │              TwoWheelsVehicle, NoParkingSign, UPILLAR, Stone_Piers}
  ├─ 找到 → 取 4 个顶点 Point_1..Point_4 → Data[0..3]
  │        DataNum=4, i++, 继续找下一个 Square
  └─ 没找到 → j++, i=0, 进入 j=1
            ↓
j=1  (Polygon 多边形物体)
  ├─ i=0..Polygon.ObjNum
  ├─ 找 Label = Curb (路边石)
  ├─ 找到 → 取所有顶点 Point[k] → Data[0..N-1]
  │        DataNum=PointNum, i++, 继续找下一个
  └─ 没找到 → j++, i=0, 进入 j=2
            ↓
j=2  (Done)
  └─ bSearch = FALSE, 退出 while
```

> **注意**：Triangle (j=1) 和 CirCular (j=2) 在源码里被 `#if 0` 注释掉了，实际只处理 Square 和 Polygon。

### 4.7 "最坏情况"累积逻辑

```c
PreOffsetY = OffsetY;       // 备份上一轮
PreOffsetX = OffsetX;
APAMAP_GetSlotBdPtOffsetByGivenObjPts(... , &OffsetX, &OffsetY, ...);
if (PreOffsetX < OffsetX) {
    ODInSlotPtForOffsetX = OffsetXRefPt;  // 记录"罪魁祸首"
}
if (PreOffsetY < OffsetY) {
    ODInSlotPtForOffsetY = OffsetYRefPt;
}
```

> **取最大**策略：每算出一个新障碍物的偏移，只在新值更大时才更新。`ODInSlotPtForOffset*` 记录触发最大偏移的那个障碍物点，用于调试/可视化。

### 4.8 输出过滤逻辑

```c
if ((OffsetX > 50) || (OffsetY > 50)) {
    if (OffsetX < 0) OffsetX = 0;
    *pOffsetX = OffsetX;
    *pOffsetY = OffsetY;
} else {
    *pOffsetX = 0;
    *pOffsetY = 0;
}
APAMap_GInfo.SlotPar.ODPt[Bordpttype] = ODInSlotPtForOffsetX;
```

- **< 50 视为 0**：微小入侵忽略不计（噪声过滤）
- **OffsetX < 0 钳到 0**：负值（向内偏移过大导致无空间）也清零
- **副作用**：把"罪魁祸首"点写到 `SlotPar.ODPt[Bordpttype]`，可在可视化时高亮这个障碍

---

## 五、三个函数的协作关系图

```
                       函数① (融合主函数)
                       ┌──────────────────────────────────────┐
                       │ 1. 收集三类偏移:                      │
                       │    SensorOffset   ──┐                │
                       │    FSDOffset      ──┤                │
                       │    ODOffset       ──┘                │
                       │           ↓                           │
                       │ 2. 取 max 融合 (各类入侵的最坏情况)   │
                       │           ↓                           │
                       │ 3. 车位净距安全检查                   │
                       │           ↓                           │
                       │ 4. 修正 Obj1/Obj2 边界点              │
                       │    (平移相交法)                        │
                       │           ↓                           │
                       │ 5. 重算 EndPos 调用                   │
                       │    APAMap_ParkingOutSetEndCarPosInOldCorSys
                       │           ↓                           │
                       │ 6. 写回全局 (SlotPar, NewCordSysOPt) │
                       └──────────┬───────────────────────────┘
                                  │
                                  │ 调用
                                  ▼
                       函数② (OD 偏移 wrapper)
                       ┌──────────────────────────────────────┐
                       │ APAMAP_ParkingOutGetSlotBdPtByODObjs  │
                       │   (0, &ODOffsetX1, &ODOffsetY1)       │
                       │ APAMAP_ParkingOutGetSlotBdPtByODObjs  │
                       │   (1, &ODOffsetX2, &ODOffsetY2)       │
                       └──────────┬───────────────────────────┘
                                  │
                                  │ 调用
                                  ▼
                       函数③ (OD 障碍物遍历)
                       ┌──────────────────────────────────────┐
                       │ while (遍历):                         │
                       │   j=0: 找 Square 警示类障碍物         │
                       │   j=1: 找 Polygon Curb 路边石         │
                       │   j=2: Done                            │
                       │   每找到 1 个:                          │
                       │     调 GetSlotBdPtOffsetByGivenObjPts │
                       │     取最大 OffsetX/Y                    │
                       │   记录"罪魁祸首"点 ODInSlotPt          │
                       └──────────────────────────────────────┘
```

---

## 六、关键数学/几何约定速查

| 项 | 值/约定 |
|----|---------|
| 车位坐标系原点 | `APAMap_GInfo.NewCordSysOPt` (OrgPt) |
| 车位坐标系 X' 方向 | `APAMap_GInfo.NewCordSysAng` (OrgAng) |
| Obj1/2 方向 | `SlotPar.Obj1Ang` / `Obj2Ang` |
| 车位在左/右侧 | `SlotPar.bSlotDataAtRigthSide` |
| `Bordpttype` 约定 | 0=Obj1（车位内侧）, 1=Obj2（车位外侧） |
| Sensor 入侵量 | `APAMap_CalSlotBorderPtOffsetBySensorMapInfo` |
| FSD 入侵量 | `APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo` |
| OD 入侵量 | 函数② → 函数③ 链 |
| 融合策略 | **各类 max 合并** |
| 入侵过滤 | OD < 50 mm 视为 0 |
| 安全净距阈值 | 2 × 250 = 500 mm |
| 修正几何 | 平移相交法 (OffsetX 平移 OrgAng 线 + OffsetY 平移 ObjAng 线 → 交点) |
| 短车位保护 | PARALLEL: SlotLen < 车长+700 → OffsetY=0 |
| 极小车位 | `bShortestSlotLen=T` → Obj2 偏移再减 150 mm |

---

## 七、可挑出来的重构建议

1. **数据融合是写死的 `max` 链**（334-404）— 35 行 if-else 链。可抽成 `MaxIfGreater(&a, b)` 宏或一个函数 `Max4(&out, in1, in2, in3, in4)`，主函数瘦身。

2. **Step ⑧⑨ 修正 Obj1/2 的代码大量复制**（537-566 与 568-623）— 平行线平移 + 交点求法可以抽成 `ShiftBorderPointByOffset(ObjPt, ObjAng, OrgAng, OffsetX, OffsetY, &OutObjPt)`。

3. **Step ⑫ 车位长分级是硬编码阈值**（630-655）— `LengthOfCar+2000/1500/1100` 这些经验值应集中到 `kParallelSlotLengthBoundaries` 数组里，便于调参。

4. **Step ⑬ 终点重算的 `#ifdef SUPPORT_PARKING_OUT_UWB` 分支可以抽函数**（668-697）— 主函数只调 `ComputeEndPos(...)`，内部按配置分发。

5. **函数② 完全可以 inline**（10 行的 wrapper）— 直接在函数①里调两次函数③，或改用数组循环。

6. **函数③ 状态机 `j` 可以用 `for` 循环 + 形状描述数组** 替代硬编码：
   ```c
   struct ObjShapeType { APA_ENUM_TYPE j; const char* name; ... };
   static const ObjShapeType kShapes[] = {{0, "Square", ...}, {1, "Polygon", ...}};
   ```

7. **函数③ 状态机 `j` 的注释掉的 Triangle/Circle 分支**（6742-6805）— 应彻底删除或用 `#if 0` 包裹整个段，避免误导。

8. **全局状态写回散落**（732-781）— 函数①末尾集中写回 7+ 个 `APAMap_GInfo.SlotPar.*` 字段，可抽 `CommitSlotBorderResult(Obj1Pt, Obj2Pt, EndPos, ...)` 函数。

9. **debug 日志字符串超过 1000 字符**（多处 snprintf）— 抽成 `LOG_SLOT_BORDER_RESULT(...)` 宏，避免主函数被日志淹没。

10. **静态标志位** (`bCarryOutSlot` 等大量 file-scope 全局变量) — 这些 `bxxxFlag` 全部定义在 `MapParkingOut.cpp` 文件头（11-27 行），建议打包成 `struct ParkOutFlag { ... }; static ParkOutFlag g_park_out_flags;` 便于模块化和测试。

---

## 八、参考文件

- **源文件**：`/media/disk/2818_proj/local_parkout/src/pnc_map/MapParkingOut.cpp`（TSD-Header 编码）
  - 函数①: 第 274-783 行
  - 函数②: 第 6882-6891 行
  - 函数③: 第 6573-6881 行
- **已分析的姐妹函数**：[Endpos定位相关.md](Endpos定位相关.md) — `APAMapParkingOutSetEndCarPosInOldCorSys`
- **Gemini 风格规范**：`~/.claude/projects/-media-disk-2818-proj-dev-Loc/memory/gemini_refactor_style.md`
