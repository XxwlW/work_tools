# 车位边界点融合 相关函数分析

> **源文件**：`src/pnc_map/MapParkingOut.cpp`
> **分析日期**：2026/06/01
> **三个函数规模合计**：约 830 行

---

## 〇、三个函数总览与调用关系

| 函数 | 行号 | 行数 | 作用 | 调用方 |
|------|------|------|------|--------|
| `APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo` | 274–783 | 510 | **总入口**：融合 FSD/Sensor/OD 三类感知数据，刷新车位边界点 Obj1/Obj2 与终点 | `APAMAP_ParkingOutCalSlotBdPtByMapInfo` 等 |
| `APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo` | 6882–6891 | 10 | **简单包装**：调 `APAMAP_ParkingOutGetSlotBdPtByODObjs` 两次（Obj1/Obj2） | Function 1 |
| `APAMAP_ParkingOutGetSlotBdPtByODObjs` | 6573–6881 | 309 | **OD 障碍物遍历**：扫 OD 列表算 Obj1/Obj2 方向上的补偿量 | Function 2 |

```
┌────────────────────────────────────────────────────────────────────┐
│  APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo (510行, 总入口) │
│  ┌──────────────────────────────────────────────────────────┐      │
│  │ ① 读全局 SlotPar/EndPos/车位坐标系                         │      │
│  │ ② 调 3 个子函数取补偿量:                                    │      │
│  │     APAMap_CalSlotBorderPtOffsetBySensorMapInfo            │      │
│  │     APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo         │      │
│  │     APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo ─────┼──┐   │
│  │ ③ 补偿量取大 (max 融合)                                      │  │   │
│  │ ④ 用平行线+交点重算 Obj1/Obj2 物理坐标                       │  │   │
│  │ ⑤ 重算 EndPos (调 APAMap_ParkingOutSetEndCarPosInOldCorSys) │  │   │
│  │ ⑥ 写回 APAMap_GInfo.SlotPar.*                              │  │   │
│  └──────────────────────────────────────────────────────────┘  │   │
└─────────────────────────────────────────────────────────────────┼───┘
                                                                   │
                                                                   ▼
                              ┌──────────────────────────────────────────┐
                              │ APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo (10行)│
                              │   包装层, 无逻辑                                          │
                              └────────────────────┬─────────────────────┘
                                                   │ 调 2 次 (Bordpttype=0/1)
                                                   ▼
                              ┌──────────────────────────────────────────┐
                              │ APAMAP_ParkingOutGetSlotBdPtByODObjs (309行) │
                              │  遍历 OD 对象 (Square/Polygon),                │
                              │  对每个对象调 APAMAP_GetSlotBdPtOffsetByGivenObjPts│
                              │  取 max offset 返回 + 记"罪魁祸首"点             │
                              └──────────────────────────────────────────┘
```

---

## 一、函数 1 详解：`APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo`

### 1.1 函数签名

```c
BOOLEAN APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo(void)
```

**返回值**：`BOOLEAN`（`TRUE`=成功 / `FALSE`=车位宽度过窄失败，错误码 58）

**作用**：把 FSD（自由空间检测）、Sensor、OD（障碍物）三类感知的补偿量融合，重算车位边界点 Obj1/Obj2、终点 EndPos、终点线 EndPosLine，写回全局 `APAMap_GInfo.SlotPar`。

### 1.2 变量分组（原名 → 重构版命名）

| 原名 | 重构版 | 含义 |
|------|--------|------|
| `Obj2Pt` / `Obj1Pt` | `object2_point` / `object1_point` | 车位外/内侧边界点（世界系） |
| `SlotLen` | `slot_length` | 车位长度 |
| `fDis1` / `fDis2` | `inner_y_distance_1/2` | Obj1/Obj2 方向上的"内 Y 距离" |
| `EndPos` / `EndPosLine` | `end_position` / `end_position_line` | 终点位置 / 终点线（Ax+By+C） |
| `DefaultOffsetY1/Y2` | `default_y_offset_1/2` | 默认 Y 补偿（无障碍时的最小余量） |
| `FSDOffsetX1/Y1/X2/Y2` | `fsd_offset_x1/y1/x2/y2` | FSD 给出的 4 个补偿量 |
| `ODOffsetX1/Y1/X2/Y2` | `od_offset_x1/y1/x2/y2` | OD 给出的 4 个补偿量 |
| `SensorOffsetX1/Y1/X2/Y2` | `sensor_offset_x1/y1/x2/y2` | 传感器融合给出的 4 个补偿量 |
| `OffsetX1/Y1/X2/Y2` | `fused_offset_x1/y1/x2/y2` | 三类取大融合后的最终补偿 |
| `NewDis1/NewDis2/NewDis` | `new_distance_1/2/total` | 减补偿后的剩余距离 |
| `Line` / `TempLine1/2` | `slot_line` / `temp_line_1/2` | 临时构造的直线 |
| `SafeDis` | `safe_distance` | 安全距离（250 mm） |
| `TempDis` / `TempDis1` | `temp_distance` | 临时距离 |
| `TempCarPos1/2` | `temp_car_pose_1/2` | 用于构造直线的临时车姿态 |
| `OrgAng` | `anchor_angle` | 车位坐标系 X' 方向角 |
| `OrgPt` | `anchor_origin` | 车位坐标系原点 |
| `bSlotDataAtRigthSide` | `is_slot_data_at_right` | 车位数据是否在右侧 |
| `ParkOutMode` | `park_out_mode` | 泊出模式 |
| `CurCarCoordinateX` | `current_car_x` | 当前车 X 坐标（米） |
| `TempPt` | `temp_point` | 临时点 |
| `bUpdataDefaulBordenFlag` | `should_apply_default_border` | 是否回退默认边界 |
| `BloundaryOffsetY` | `boundary_offset_y` | 边界 Y 补偿（用于短车位特殊修正） |
| `bSeizeEndCarPosFlag` | `is_end_pos_invaded_by_fsd` | FSD 是否侵占终点 |

### 1.3 函数流程（8 段）

```
   ┌──────────────────────────────────────────────────────────────┐
   │ 段①  变量声明 + 取全局状态 (274-332)                          │
   │       Obj2Pt, Obj1Pt, SlotLen, EndPos, EndPosLine,           │
   │       OrgAng, OrgPt, ParkOutMode, CurCarCoordinateX          │
   │       初始化: bSeizeEndCarPosFlag=FALSE, SafeDis=250         │
   └──────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
   ┌──────────────────────────────────────────────────────────────┐
   │ 段②  收集 3 类补偿量 (334-404)                                │
   │     调 3 个子函数, 取每类 4 个 (X1,Y1,X2,Y2) 补偿              │
   │     再 max-融合: OffsetX1 = max(FSD, Sensor, OD)              │
   │     阈值过滤: ODOffsetX1 < 50 → 置 0 (噪声门限)               │
   └──────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
   ┌──────────────────────────────────────────────────────────────┐
   │ 段③  计算 fDis1/fDis2 车位内部可用空间 (407-421)              │
   │     bCarryOutSlot?                                            │
   │       TRUE  → APAMap_GetSearchMaxInnerY(...)                  │
   │       FALSE → AlgCom_GetPointToLineDis(...) - 保险杠距离      │
   └──────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
   ┌──────────────────────────────────────────────────────────────┐
   │ 段④  决定是否"回退默认边界" (423-485)                          │
   │     bAfterNewAnchorPointFlag + 当前车 X 位置 + 模式判定        │
   │     → bUpdataDefaulBordenFlag                                  │
   │     TRUE  → OffsetX1=X2=Y1=Y2=0 (回退)                       │
   │     FALSE → DefaultOffsetY1/Y2 计算 (PARALLEL 时为 0)          │
   └──────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
   ┌──────────────────────────────────────────────────────────────┐
   │ 段⑤  安全距离检查 (487-523)                                   │
   │     非 USSLOT 模式: NewDis1+NewDis2 < 2*SafeDis → 失败 58     │
   │     USSLOT 模式: 跳过此检查                                    │
   └──────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
   ┌──────────────────────────────────────────────────────────────┐
   │ 段⑥  短车位特殊处理 (525-535)                                 │
   │     PARALLEL: SlotLen < 车长+700 → Y 补偿清零                  │
   │     其他:    SlotLen < 车宽+500 → Y 补偿清零                    │
   └──────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
   ┌──────────────────────────────────────────────────────────────┐
   │ 段⑦  平行线+交点 重算 Obj1/Obj2 (537-623)                     │
   │     对每个 obj:                                                │
   │       1. 过该点沿 ObjAng 方向作基线                             │
   │       2. 用 OffsetY 做平行线                                    │
   │       3. 用 OffsetX 做另一组平行线                              │
   │       4. 两条平行线交点 = 新 Obj 点                              │
   │     边界点侵占检查: APAMap_ParkingOutCarPosInvadeSlotBorderInfo│
   │     重算 SlotLen: 沿 Obj2Ang 方向的 Obj1 距离                  │
   └──────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
   ┌──────────────────────────────────────────────────────────────┐
   │ 段⑧  终点重算 + 写回全局 (657-783)                            │
   │     条件: 已过新锚点 / 车道线 / 参考线 → 不重算 EndPos          │
   │     否则: 调 APAMap_ParkingOutSetEndCarPosInOldCorSys         │
   │     EndPos.x = 0xff → 失败 101                                │
   │     写回: bObj1Exist/bObj2Exist/Obj1Pt/Obj2Pt/SlotBordPt     │
   │           SlotLen/EndPos/EndPosLine                           │
   └──────────────────────────────────────────────────────────────┘
```

### 1.4 补偿量融合（段 ② 关键逻辑）

```
  SensorOffsetX1/Y1/X2/Y2
              ↓
  FSDOffsetX1/Y1/X2/Y2      (APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo)
              ↓
  ODOffsetX1/Y1/X2/Y2       (APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo)
              ↓
  ┌─ 阈值过滤 ─────────────────┐
  │ ODOffsetX < 50 → 0         │   噪声门限
  └────────────────────────────┘
              ↓
  ┌─ Max 融合 ─────────────────┐
  │ OffsetX1 = max(FSD, Sensor, OD)│
  │ OffsetX2 = max(FSD, Sensor, OD)│
  │ OffsetY1 = max(FSD, Sensor, OD)│
  │ OffsetY2 = max(FSD, Sensor, OD)│
  └────────────────────────────┘
              ↓
       FusedOffsetX1/Y1/X2/Y2
```

**关键点**：
- 三类源**独立计算**，最后**取最大值**（保守策略：哪个最严就听哪个）
- OD 有 50 mm 噪声门限（小于阈值的视为无）
- FSD 当前走的是 `#if 1` 分支，`#else` 是调试用 0 值分支

### 1.5 段 ④ "回退默认边界" 决策表

```
bAfterNewAnchorPointFlag  ==  TRUE?  (车已驶出车位, 进入新锚点之后)
   │
   ├─ FALSE → 不进入该段, bUpdataDefaulBordenFlag = FALSE
   │
   └─ TRUE
        │
        ├─ bSlotDataAtRigthSide?  →  CurCarCoordinateX 取反
        │
        └─ ParkOutMode ?
             │
             ├─ HEAD_TURN_ROUND:    CurCarCoordinateX > -1 ?  TRUE
             ├─ REAR_TURN_ROUND:    CurCarCoordinateX >  2 ?  TRUE
             └─ 其他 (PARALLEL等):  CurCarCoordinateX >  0 ?  TRUE
```

**含义**：车已驶出车位后，如果离车位足够远，就**不再用感知数据修正边界点**，回退到默认几何边界，避免不必要的扰动。

### 1.6 段 ⑦ 平行线交点构造 Obj 点的几何示意

```
        Y'↑
         |
   Y'方向 |     Obj1Ang 平行线 (TempLine2)
         |    ╱─────────●─────────  (偏移 OffsetY1)
         |   ╱         Obj1Pt (新)
         |  ╱
         | ╱  OrgAng 平行线 (TempLine1)
         |╱─────────────●─────────  (偏移 OffsetX1)
         ●Obj1Pt (旧)
         |
         +────────────────────→ X'

   两条平行线的交点 = 新 Obj1Pt
   - TempLine1: 沿 OrgAng 方向, 距旧 Obj1Pt 的 X' 偏移 OffsetX1
   - TempLine2: 沿 Obj1Ang 方向, 距旧 Obj1Pt 的 Y' 偏移 OffsetY1
```

### 1.7 段 ⑧ 终点重算的分流

```c
if (bAfterNewAnchorPointFlag || bLaneLineUpdateEndCarPosFlag || bRefercLineUpdateEndCarPosFlag) {
    // 满足以上任一条件 → 不更新 EndPos, 沿用旧值
    EndPos = APAMap_GInfo.SlotPar.EndPos;
} else {
    // 正常分支: 调 APAMap_ParkingOutSetEndCarPosInOldCorSys
    EndPos.Coordinate = APAMap_ParkingOutSetEndCarPosInOldCorSys(
        ParkOutMode, OrgPt, OrgAng, Obj2Pt, bSeizeEndCarPosFlag);
    // x = 0xff 表示直出模式垂直线异常
    if (EndPos.Coordinate.x == 0xff) {
        APAMAP_Setfailcause(101);
        return FALSE;
    }
}
```

### 1.8 异常路径

| 错误码 | 触发条件 | 位置 |
|-------|---------|------|
| **58** | 非 USSLOT 模式 + 车位可用空间 `NewDis1+NewDis2 < 2*250 mm` | 段 ⑤ |
| **101** | 直出模式垂直线 / `EndPos.x = 0xff` | 段 ⑧ |

---

## 二、函数 2 详解：`APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo`

### 2.1 函数签名

```c
void APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo(
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY1,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX2,
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY2)
```

### 2.2 完整函数体

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

### 2.3 作用

**纯包装层**，无任何业务逻辑：
- 调 `APAMAP_ParkingOutGetSlotBdPtByODObjs` **两次**（`Bordpttype=0`→Obj1, `Bordpttype=1`→Obj2）
- 把两次的结果分别填入 4 个 out 参数

> 命名上 `CalSlotBorderPtOffset` 和 `GetSlotBdPtByODObjs` 是**主从关系**，但实际上**没有共享数据**，每次都从全局 `APAMap_GInfo.SlotPar` 重读。可以视作"接口拆分"的痕迹——主函数期望"两个 Obj 分别给一组补偿"，所以用包装层拆开。

### 2.4 重构建议

- 这个 10 行包装**完全可以内联到 Function 1** 的段 ②，节省一层调用栈
- 或者把 `APAMAP_ParkingOutGetSlotBdPtByODObjs` 改成 `ComputeOdOffsetsForBothBorders(OUT* pOffsetX1, OUT* pOffsetY1, OUT* pOffsetX2, OUT* pOffsetY2)`，避免两次重复读取全局

---

## 三、函数 3 详解：`APAMAP_ParkingOutGetSlotBdPtByODObjs`

### 3.1 函数签名

```c
void APAMAP_ParkingOutGetSlotBdPtByODObjs(
    APA_ENUM_TYPE Bordpttype,             // 0=Obj1, 1=Obj2
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetX,// 输出: X 补偿
    APA_DISTANCE_CAL_FLOAT_TYPE* pOffsetY)// 输出: Y 补偿
```

### 3.2 变量分组（原名 → 重构版命名）

| 原名 | 重构版 | 含义 |
|------|--------|------|
| `i`, `j`, `k` | `square_idx`, `shape_state_idx`, `pt_idx` | 循环索引 |
| `OffsetX` / `OffsetY` | `current_offset_x/y` | 当前计算的 X/Y 补偿量 |
| `TargetXLoc` / `TargetYLoc` | `search_dir_x/y` | 搜索方向（左/右） |
| `LineXStrPt` / `LineXEndPt` | `axis_x_start_pt/end_pt` | X 方向搜索线起止点 |
| `LineYStrPt` / `LineYEndPt` | `axis_y_start_pt/end_pt` | Y 方向搜索线起止点 |
| `Data[10]` | `obstacle_pts[10]` | 当前障碍物的轮廓点 |
| `DataNum` | `obstacle_pt_count` | 障碍物轮廓点数量 |
| `MaxOutOffsetX` | `max_outward_offset_x` | X 方向外扩上限 (2000) |
| `MaxInnerOffsetX` | `max_inner_offset_x` | X 方向内缩上限 (1500/2800) |
| `MaxOutOffsetY` | `max_outward_offset_y` | Y 方向外扩上限 (1000) |
| `MaxInnerOffsetY` | `max_inner_offset_y` | Y 方向内缩上限（动态算） |
| `pODInfo` | `od_info` | OD 全局数据指针 |
| `bSearch` | `is_searching` | 主循环标志 |
| `OrgAng` / `OrgPt` | `anchor_angle` / `anchor_origin` | 车位坐标系 |
| `ObjAng` | `object_angle` | 目标 Obj 方向角 |
| `bDataAtRightSide` | `is_slot_data_at_right` | 车位在右侧 |
| `PreOffsetX` / `PreOffsetY` | `prev_offset_x/y` | 上一轮循环的 offset（用于比较） |
| `ODInSlotPtForOffsetX/Y` | `od_point_for_offset_x/y` | 触发最大 offset 的"罪魁祸首"点 |
| `OffsetXRefPt` / `OffsetYRefPt` | `offset_x_ref_pt` / `offset_y_ref_pt` | 当前轮算出的关键点 |
| `TempDis` | `temp_distance` | 临时距离 |
| `CurObjComInfo` | `current_obj_info` | 当前障碍物信息 |
| `ParkOutMode` | `park_out_mode` | 泊出模式 |

### 3.3 函数流程

```
   ┌────────────────────────────────────────────────────────────┐
   │ 段①  早退检查 + 读全局 (6611-6637)                          │
   │     OD 时间戳为 0 → 直接返回 0, 0                           │
   │     pODInfo = TotalMap.ODInfo (或 VisObjs.ODInfo)           │
   │     MaxOutOffsetX=2000, MaxOutOffsetY=1000                  │
   │     MaxInnerOffsetX = 1500(PARALLEL) / 2800(其他)          │
   └──────────────────────────┬─────────────────────────────────┘
                              │
                              ▼
   ┌────────────────────────────────────────────────────────────┐
   │ 段②  设置搜索方向 (6638-6660)                               │
   │     Bordpttype=0 → Obj1Ang                                 │
   │     Bordpttype=1 → Obj2Ang                                 │
   │     TargetXLoc/TargetYLoc 由 bDataAtRightSide 决定          │
   │     (车位在右 → X:左 Y:看 obj 类型)                          │
   └──────────────────────────┬─────────────────────────────────┘
                              │
                              ▼
   ┌────────────────────────────────────────────────────────────┐
   │ 段③  构造搜索线 + 算 MaxInnerOffsetY (6661-6675)            │
   │     LineY: 沿 OrgAng 方向, 1000 mm 长度                    │
   │     LineX: 沿 ObjAng 方向, 1000 mm 长度                    │
   │     MaxInnerOffsetY = APAMap_GetSearchMaxInnerY(...) - 300  │
   │     最小值 300 mm (避免负值)                                │
   └──────────────────────────┬─────────────────────────────────┘
                              │
                              ▼
   ┌────────────────────────────────────────────────────────────┐
   │ 段④  初始化 (6679-6690)                                     │
   │     i=0, j=0, k=0                                           │
   │     OffsetX = -MaxInnerOffsetX                              │
   │     OffsetY = -MaxOutOffsetY                                │
   │     PreOffsetX/Y = 0                                       │
   │     ODInSlotPtForOffsetX/Y = NO_OBJ_DISTANCE               │
   └──────────────────────────┬─────────────────────────────────┘
                              │
                              ▼
   ┌────────────────────────────────────────────────────────────┐
   │ 段⑤  状态机遍历 OD 对象 (6691-6858)                         │
   │                                                            │
   │  j=0 → Square (4 边形) 状态:                                │
   │     i 遍历 Square.ObjNum, 找匹配 Label 的对象:              │
   │       WarningPost, ConeBucket, SquareColumn,                │
   │       TwoWheelsVehicle, NoParkingSign,                      │
   │       UPILLAR, Stone_Piers                                  │
   │     找到 → 取 4 个角点 → DataNum=4                          │
   │     找完 → j++ (进下一个状态)                                │
   │                                                            │
   │  j=1 → Polygon 状态 (Road Curb):                            │
   │     i 遍历 Polygon.ObjNum, 找 Label==Curb 的对象            │
   │     找到 → 取所有顶点 → DataNum=PointNum                    │
   │     找完 → j++                                              │
   │                                                            │
   │  j=2 → 终止 (bSearch = FALSE)                               │
   │                                                            │
   │  每轮调:                                                    │
   │    APAMAP_GetSlotBdPtOffsetByGivenObjPts(...)               │
   │    取 max 更新 OffsetX/Y                                    │
   │    记录"罪魁祸首"点 ODInSlotPtForOffsetX/Y                  │
   └──────────────────────────┬─────────────────────────────────┘
                              │
                              ▼
   ┌────────────────────────────────────────────────────────────┐
   │ 段⑥  阈值过滤 + 输出 (6867-6878)                            │
   │     (OffsetX > 50) OR (OffsetY > 50) → 输出真实值           │
   │     否则 → 输出 0, 0 (无显著影响)                            │
   │     写回 APAMap_GInfo.SlotPar.ODPt[Bordpttype]              │
   │     = ODInSlotPtForOffsetX (罪魁祸首点)                      │
   └────────────────────────────────────────────────────────────┘
```

### 3.4 OD 对象状态机

```
   j=0: Square
   ├─ 筛选 Label: WarningPost / ConeBucket / SquareColumn /
   │             TwoWheelsVehicle / NoParkingSign / UPILLAR /
   │             Stone_Piers
   ├─ 取 4 个角点
   └─ 全部扫完 → j=1
          │
          ▼
   j=1: Polygon
   ├─ 筛选 Label: Curb (路沿石)
   ├─ 取所有顶点
   └─ 全部扫完 → j=2
          │
          ▼
   j=2: bSearch = FALSE
   → 退出 while 循环
```

> ⚠️ **注意**：`#if 0 ... #else ... #endif` 把 Triangle / CirCular 状态关掉了（6742-6837），目前**只处理 Square 和 Polygon**。注释里 `#if 0` 的旧代码仍在（6805-6805），但不会编译进。

### 3.5 搜索方向决策表（段 ②）

| `bDataAtRightSide` | `Bordpttype` | TargetXLoc | TargetYLoc | 含义 |
|--------------------|--------------|-----------|-----------|------|
| TRUE | 0 (Obj1) | 0 (左) | 1 (右) | 车位在右, Obj1 在远端 |
| TRUE | 1 (Obj2) | 0 (左) | 0 (左) | 车位在右, Obj2 在近端 |
| FALSE | 0 (Obj1) | 1 (右) | 0 (左) | 车位在左, Obj1 在远端 |
| FALSE | 1 (Obj2) | 1 (右) | 1 (右) | 车位在左, Obj2 在近端 |

### 3.6 关键算法：`APAMAP_GetSlotBdPtOffsetByGivenObjPts`

此函数在 Function 3 段 ⑤ 中被反复调用，签名：

```c
APAMAP_GetSlotBdPtOffsetByGivenObjPts(
    TargetXLoc, TargetYLoc,         // 搜索方向 (0/1)
    &LineXStrPt, &LineXEndPt,       // X 方向基线
    &LineYStrPt, &LineYEndPt,       // Y 方向基线
    &Data[0], DataNum,              // 当前障碍物轮廓
    MaxOutOffsetX, MaxInnerOffsetX, // 边界
    MaxOutOffsetY, MaxInnerOffsetY, // 边界
    &OffsetX, &OffsetY,             // 输入输出: 累积 max offset
    &OffsetYRefPt, &OffsetXRefPt)   // 输出: 触发 max 的"罪魁祸首"点
```

**核心思想**：
- 在 X/Y 方向各有"内缩上限"和"外扩上限"
- 障碍物越靠近基线，offset 越大（外扩/内缩更多）
- 多轮调用取 **max** —— 找到最"危险"的那个障碍物

### 3.7 "罪魁祸首"点记录

```c
if (PreOffsetX < OffsetX) {  // 本轮 offset 更大 → 记为新"罪魁祸首"
    ODInSlotPtForOffsetX = OffsetXRefPt;
}
if (PreOffsetY < OffsetY) {
    ODInSlotPtForOffsetY = OffsetYRefPt;
}
```

最终 `ODInSlotPtForOffsetX` 写到 `APAMap_GInfo.SlotPar.ODPt[Bordpttype]`，供后续日志/调试用。

---

## 四、关键全局状态依赖

| 全局变量 | 来源 | 用法 |
|---------|------|------|
| `APAMap_GInfo.SlotPar.Obj1Pt` / `Obj2Pt` | 全局 | Function 3 搜索线起点 |
| `APAMap_GInfo.SlotPar.Obj1Ang` / `Obj2Ang` | 全局 | Function 3 搜索方向 |
| `APAMap_GInfo.NewCordSysAng` / `NewCordSysOPt` | 全局 | 车位坐标系 |
| `APAMap_GInfo.SlotPar.bSlotDataAtRigthSide` | 全局 | 方向翻转 |
| `APAMap_GInfo.SlotPar.EndPos` / `EndPosLine` | 全局 | 终点读/写 |
| `APAMap_GInfo.SlotPar.ODPt[0/1]` | 全局 | OD 罪魁祸首点（Function 3 写） |
| `g_park_out_status.*` | 全局 | `bAfterNewAnchorPointFlag` / `bLabelAngledFlag` / `bCntAddFlag` 等 |
| `APAMap_GInputData.TotalMapInfo.mapData.ODInfo` | 输入 | OD 对象列表 |
| `APAMap_GInputData.CarLocInfo.CarPos` | 输入 | 当前车位置 |
| `APAMap_GInputData.ParkReqPar.parkoutmode` | 输入 | 泊出模式 |

---

## 五、跨函数决策表汇总

### 5.1 三类补偿量来源对比

| 来源 | 函数 | 输入 | 输出 | 特殊过滤 |
|------|------|------|------|---------|
| **Sensor** | `APAMap_CalSlotBorderPtOffsetBySensorMapInfo` | 传感器融合数据 | `SensorOffsetX1/Y1/X2/Y2` | 无 |
| **FSD** | `APAMap_CalSlotBorderPtOffsetByTopViewFSDMapInfo` | 视觉 FSD 地图 | `FSDOffsetX1/Y1/X2/Y2` | `#if 1` 启用 / `#else` 全 0 |
| **OD** | `APAMap_ParkingOutCalSlotBorderPtOffsetByODMapInfo` (包装) → `APAMAP_ParkingOutGetSlotBdPtByODObjs` (实现) | OD 对象列表 | `ODOffsetX1/Y1/X2/Y2` | 50 mm 噪声门限 |

### 5.2 段 ① 与 段 ② 状态机合并

Function 1 段 ① 中 `bCarryOutSlot` 决定如何算 `fDis1/fDis2`：
- TRUE → `APAMap_GetSearchMaxInnerY`（车位有内边界定义）
- FALSE → `AlgCom_GetPointToLineDis` - 保险杠距离（车位无内边界，从车位置反推）

### 5.3 段 ⑦ 平行线构造的"对偶性"

每个 Obj 点都做相同的"两次平行线 + 一次交点"操作：

```
              ObjAng 方向平行线
              ╱
             ╱
   旧点 ●───●───  (OffsetY 偏移)
   ╱
  ╱ OrgAng 方向平行线
  ╱
 ●─── (OffsetX 偏移)
 
 两条平行线交点 = 新点
```

短车位 (`bShortSlotLen`) 还会强制 `OffsetX2 = 0`，避免把车位缩得更小。

---

## 六、异常路径与错误码

| 错误码 | 函数 | 触发条件 | 影响 |
|-------|------|---------|------|
| **58** | Function 1 | 非 USSLOT + `NewDis1+NewDis2 < 2*SafeDis` (500 mm) | `return FALSE` |
| **101** | Function 1 | `EndPos.x = 0xff`（直出垂直线异常） | `return FALSE` |

哨兵值：
- `ODInSlotPtForOffsetX/Y = (NO_OBJ_DISTANCE, NO_OBJ_DISTANCE)` 表示无有效 OD 点
- `EndPos.x = 0xff` 表示终点计算失败

---

## 七、重构建议

1. **抽 `bCarryOutSlot` / `bAfterNewAnchorPointFlag` 等魔法标志为状态机**
   - 当前散落在段 ①/段 ④ 的 `bAfterNewAnchorPointFlag`、`bLabelAngledFlag`、`bUpdataDefaulBordenFlag` 互相耦合
   - 建议：定义为 enum 状态，由段首的判定函数输出，后续只读

2. **拆段 ② 的 max 融合为 `MergeThreeSourceOffsets(FSD, Sensor, OD) -> Fused`**
   - 80 行融合逻辑提到子函数，Function 1 主线变薄
   - 50 mm 噪声门限可以参数化

3. **抽 `MakeObjPointByParallelLines(OrgAng, ObjAng, OldObjPt, OffsetX, OffsetY, bSlotAtRight, *NewObjPt)`**
   - 段 ⑦ 中 Obj1/Obj2 的平行线构造逻辑完全对称，重复 2 次
   - 抽成 helper 后段 ⑦ 缩一半

4. **包装层 Function 2 可以内联到 Function 1**
   - 仅 10 行，没有共享状态
   - 直接在 Function 1 段 ② 中写两次调用，少一层栈

5. **Function 3 的状态机 `j = 0/1/2` 建议重写为 switch + 显式状态 enum**
   - 当前用 `j` 整数 + `if (j == 0) { if (j == 1) { if (j == 2) {...} } }` 嵌套
   - 改成 `enum { SHAPE_SQUARE, SHAPE_POLYGON, SHAPE_DONE } state;` 后可读性大幅提升

6. **三元 if-else 链 `Bordpttype == 0 ? Obj1Ang : Obj2Ang` 建议预存为 `ObjAng = (Bordpttype==0) ? ... : ...`**
   - 当前 6638-6643 行用 if-else 不必要

7. **OD 罪魁祸首点只在 `#ifdef DEBUG_PRINT_SLOTOBJ` 下日志**
   - 段 ⑥ 写了 `APAMap_GInfo.SlotPar.ODPt[Bordpttype]`，但 release 模式下无法回查
   - 建议：信息密度低，可无条件记日志或加 ring buffer

8. **大量魔法数字集中到宏**
   - 250 (SafeDis), 2000 (MaxOutOffsetX), 1000 (MaxOutOffsetY), 1500/2800 (MaxInnerOffsetX), 300, 600, 100, 50
   - 与 Gemini 风格规范保持一致，集中到文件顶部的 `#define` 区域

---

## 八、参考文件

- **源文件**：`/media/disk/2818_proj/local_parkout/src/pnc_map/MapParkingOut.cpp`
  - Function 1：行 274–783
  - Function 2：行 6882–6891
  - Function 3：行 6573–6881
- **重构对照**：`/media/disk/2818_proj/local_parkout/record_code_new/MapParkingOut_refactored.cpp`
- **Gemini 风格规范**：`~/.claude/projects/-media-disk-2818-proj-dev-Loc/memory/gemini_refactor_style.md`
- **相关分析**：
  - [Endpos定位相关.md](Endpos定位相关.md) — `APAMap_ParkingOutSetEndCarPosInOldCorSys` 详解
