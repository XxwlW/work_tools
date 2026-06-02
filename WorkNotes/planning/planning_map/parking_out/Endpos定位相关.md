# APAMapParkingOutSetEndCarPosInOldCorSys 函数分析

> **源文件**：`src/pnc_map/park_out_pnc_map.cpp` 433-668 行
> **分析日期**：2026/06/01
> **函数规模**：约 235 行，7 段代码
> **重构对照**：`SetEndCarPos_refactored.cpp`（Gemini 风格）

---

## 一、函数签名与作用

```c
APACoordinateDataCalFloatType APAMapParkingOutSetEndCarPosInOldCorSys(
    uint8_t_INF      park_out_mode,         // 泊出模式
    APACoordinateDataCalFloatType anchor_pt,// 车位坐标系原点
    APA_DISTANCE_CAL_FLOAT_TYPE   anchor_ang,// 车位坐标系 X' 方向角
    APACoordinateDataCalFloatType obj2_pt,  // 车位外侧边界点
    BOOLEAN                       is_end_pos_invaded)
```

**作用**：在**车位局部坐标系 (X',Y')** 下算出 `park_out_mode` 决定的终点位置，再反变换到**世界坐标系 (X,Y)** 返回。FSD 入侵车位边界时自动加补偿。

**坐标系约定**：
- Local X' = `anchor_ang` 方向 = 车位长边方向
- Local Y' = 垂直于车位长边，指向车位外侧
- 转换工具：
  - `AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(world_pt, 0, anchor_ang, anchor_pt)` → World→Local
  - `AlgCom_PointPosWithAngAndCenterPt(local_pt, anchor_ang, anchor_pt)` → Local→World

---

## 二、函数总流程图

```
┌──────────────────────────────────────────────────────────────┐
│ 入口: park_out_mode, anchor_pt, anchor_ang, obj2_pt,         │
│       is_end_pos_invaded                                     │
└─────────────────────┬────────────────────────────────────────┘
                      │
                      ▼
        ┌─────────────────────────────┐
        │ ① 取全局状态                │
        │   cur_car_pos               │
        │   prev_end_car_pos          │
        │   park_out_env              │
        └────────────┬────────────────┘
                      │
                      ▼
        ┌─────────────────────────────┐
        │ ② APA 状态切换 → 重置      │
        │   is_end_pos_initialized=T  │
        └────────────┬────────────────┘
                      │
                      ▼
        ┌─────────────────────────────┐
        │ ③ 计算 FSD 入侵补偿量       │
        │   boundary_intrusion_offset │
        │   straight_mode_offset      │
        └────────────┬────────────────┘
                      │
        ┌─────────────┴─────────────┐
        │                           │
        ▼                           ▼
  ┌────────────┐            ┌────────────┐
  │ ④ 非直出   │            │ ⑤ 直出     │
  │ PARALLEL / │            │ HEAD_GO /  │
  │ ANGLED /   │            │ REAR_GO    │
  │ PERPEND.   │            │ STRAIGHT   │
  └─────┬──────┘            └──────┬─────┘
        │                          │
        │ 算 default_end_pos       │ 算 trajectory_line
        │  (X',Y')                 │  代入 X' 直线方程
        │                          │
        └────────────┬─────────────┘
                      │
                      ▼
        ┌─────────────────────────────┐
        │ ⑥ else 分支叠加 obj2_pt    │
        │   偏置 (slot_at_right 判定) │
        └────────────┬────────────────┘
                      │
                      ▼
        ┌─────────────────────────────┐
        │ ⑦ Local → World 反变换     │
        │   写回 EndPos.CarAng       │
        │   返回 final_end_pos_world │
        └─────────────────────────────┘
```

---

## 三、变量名对照表（原名 → 注释原始名 → 重构版名 → 物理含义）

| 原名 | 注释中标注 | 重构版 | 含义 |
|------|-----------|--------|------|
| `angle_diff` | TempAng | `vehicle_to_anchor_angle` | 当前车头在 X' 下的偏角 |
| `end_pos_angle_diff` | TempAng1 | `target_end_heading_in_anchor` | 终点车头在 X' 下的偏角 |
| `end_pos_car_ang` | — | `target_end_heading` | 终点车头（世界系） |
| `cur_car_pos` | — | `current_car_position` | 当前车位置+朝向（世界系） |
| `prev_end_car_pos` | — | `current_end_position` | 上一帧终点坐标（世界系） |
| `end_pos_local` | TempPt2 | `point_in_anchor_frame` | 临时局部坐标点 |
| `final_end_pos_world` | TempPt3 | `calculated_end_position` | 最终世界坐标终点 |
| `temp_car_pos` | TempCarPos | `car_pose_in_anchor_frame` | 用于直线计算的临时车姿态 |
| `temp_car_pos_line` | TempLine | `target_path_line` | 直出方向的轨迹线 (Ax+By+C) |
| `straight_move_distance` | fDis1 | `straight_exit_target_x_mm` | 直出时终点的 X' 距离 |
| `is_end_pos_initialized` | bEndCarPosInitFlag | `is_end_position_initialized` | 终点初始化标志（static） |
| `boundary_intrusion_offset_x_mm` | — | 同 | FSD 入侵时 X' 补偿量 |
| `boundary_intrusion_offset_y_mm` | — | 同 | FSD 入侵时 Y' 补偿量 |
| `straight_mode_offset_x` | — | `straight_exit_compensation_x_mm` | 直出时 X' 额外补偿 |
| `straight_mode_offset_y` | — | `straight_exit_compensation_y_mm` | 直出时 Y' 额外补偿 |
| `default_end_pos_x` | — | `default_end_position_x_mm` | 非直出默认终点的 X' |
| `default_end_pos_y` | — | `default_end_position_y_mm` | 非直出默认终点的 Y' |

---

## 四、7 段代码逐步拆解

### 段 ① 入口与变量声明（437–477）

```c
cur_car_pos       = APAMap_GInputData.CarLocInfo.CarPos;
is_slot_at_right  = APAMap_GInfo.SlotPar.bSlotDataAtRigthSide;
is_obj2_exist     = APAMap_GInputData.SlotUpData.bObj2Exist;
prev_end_car_pos  = APAMap_GInfo.SlotPar.EndPos.Coordinate;
park_out_env      = g_park_out_status.park_out_env_interference_;
```

- **关键全局状态**一次性拷到本地，避免分支里到处查全局
- `prev_end_car_pos`：上一帧存的终点位置，用于"非首次"分支平滑过渡
- 全部中间量显式清零（471–477）

### 段 ② APA 状态切换重置（479–483）

```c
if (APAstate <= 3 && APARunningstate >= 1 && !has_passed_new_anchor_) {
    is_end_pos_initialized = TRUE;
}
```

**触发条件**：APA 状态进入"运行态"且车还没驶过新锚点。意图是**重新规划**时把终点重置回默认值，避免沿用旧值。

### 段 ③ FSD 入侵补偿量决策（485–531）

输入：`is_end_pos_invaded == TRUE`（FSD 障碍物已入侵当前计算的终点）
输出：4 个补偿量

```
默认入口值:
  boundary_intrusion_offset_x_mm = 150
  boundary_intrusion_offset_y_mm = 100
  straight_mode_offset_x         = 100

                      ┌── 入侵主/子双边界 ──┐
                      │                     │
                      ▼                     ▼
        ┌─ 主边界被入侵 ─┐         ┌─ 子边界被入侵 ─┐
        │  straight_    │         │ 边界位置在车辆外侧?
        │  mode_y = 100 │         │  → 补偿 = HalfWidthOfCar
        │  FSD 在外侧? │         │  补偿取反 (向主边界内缩)
        │  → x = HalfW │
        │  直出模式?   │
        │  → y = -y   │
        └──────┬───────┘         └────────┬────────┘
               │                          │
               └────────┬─────────────────┘
                        ▼
              ┌─ 仅主边界入侵 ─┐
              │  x 取反        │
              │  straight_y=100│
              └────────────────┘
```

**决策表**：

| 入侵情形 | 触发条件 | `boundary_intrusion_offset_x_mm` | `boundary_intrusion_offset_y_mm` | `straight_mode_offset_x` | `straight_mode_offset_y` |
|---------|---------|----------------------------------|----------------------------------|--------------------------|--------------------------|
| 默认 (单边入侵) | — | 150 | 100 | 100 | 0 |
| 双边界 + 主边界 + FSD 在外侧 | 4 个 is_* 组合 | **= HalfWidthOfCar** | 500 | 100 | ±100 (直出模式时反号) |
| 双边界 + 主边界 + FSD 在内侧 | — | 150 | 500 | 100 | 100 |
| 双边界 + 子边界 + 在外侧 | — | **HalfWidthOfCar** → **取反** | 100 | 100 | 0 |
| 单主边界入侵 | — | 150 | 100 | **-100** | 100 |

> 核心思想：**FSD 障碍物在哪里，就把终点往反方向让开**。`HalfWidthOfCar` 是"退半车宽"，让出的余量给车体用。

### 段 ④ 非直出模式默认终点（533–567）

仅当 `park_out_mode ∈ {PARALLEL, ANGLED, PERPENDICULAR}` 执行。

**首次（`is_end_pos_initialized == TRUE`）**：按车位类型查表给默认值

| 车位类型 | default X' (mm) | default Y' (mm) |
|---------|----------------|----------------|
| PARALLEL (平行) | `-(HalfWidthOfCar + 950) - offset_x` | `2000 - offset_y` |
| ANGLED 斜列（非阶梯） | `-(HalfWidthOfCar + 1100) - offset_x` | `5000 - offset_y` |
| ANGLED 斜列（阶梯） | `-(HalfWidthOfCar + 2000) - offset_x` | `5000 - offset_y` |
| PERPENDICULAR (垂直) | `-(HalfWidthOfCar + 3100) - offset_x` | `4500 - offset_y` |

**几何含义**：
- X' **负值** = 车位外侧方向
- `HalfWidthOfCar + N` 中的 N 是经验标定的"驶出距离"
- 阶梯车位（角度特别大）驶出距离加倍

**后续（`is_end_pos_initialized == FALSE`）**：

```c
end_pos_local = 把"当前车位置"转到局部
default_end_pos_y = end_pos_local.y - offset_y
default_end_pos_x = -|end_pos_local.x| - offset_x
```

> 含义：**终点跟着车走**。Y' 跟当前车一致（去掉补偿），X' 取车当前位置的绝对值再加补偿（保证仍在车位外侧）。

### 段 ⑤ 直出模式（569–636）

```
       ┌─ 算 angle_diff = cur_heading - anchor_ang
       │
       ├─ 算 straight_move_distance (X' 距离)
       │     ├─ 首次: -(500 + 悬长 - offset_x·sin|angle_diff|)
       │     └─ 后续: prev_end_pos.x + offset_x·sin|angle_diff|
       │
       ├─ 算 end_pos_car_ang
       │     └─ Obj2Ang [+π if REAR_GO_STRAIGHT]
       │
       ├─ 算 end_pos_angle_diff = end_pos_car_ang - anchor_ang
       │
       ├─ 构造 temp_car_pos (在局部坐标下, 方向 = end_pos_angle_diff)
       │
       ├─ trajectory_line = 过该点的直线
       │
       ├─ if 垂直线 (LineType == APALineIsVertical)
       │     → 报错 101, x = 0xff, return
       │
       ├─ end_pos.x = straight_move_distance
       ├─ end_pos.y = A·x + C - default_end_pos_y·sin|angle_diff|
       │                  ↑            ↑
       │                  │            default_end_pos_y 在此分支为 0
       │                  直线方程
       │
       └─ 写 EndPos.CarAng
             ├─ 已过新锚点 → 跟当前车头
             └─ 否则       → 跟车位长边方向
```

**直出几何示意**（在车位局部坐标系内）：

```
        Y'↑
         |
         |      trajectory_line (A·X' + Y' + C = 0)
         |     ╱
         |    ╱   end_pos
         |   ╱   ●━━━━━━━━━━●━→ X'
         |  ╱   ╱            │ straight_move_distance
         | ╱   ╱             │ (通常为负)
         |╱   ●cur_car_pos
         ●O (anchor_pt)

  - 直线过 cur_car_pos 局部坐标
  - 方向 = end_pos_angle_diff (REAR 时反向)
  - X' 端点 = straight_move_distance
  - Y' 端点 = A·X' + C
```

**直出 X' 距离详解**：

```
首次初始化:
  车头直出 (HEAD): -(500 + 后悬长 - offset_x·sin|θ|)
  车尾直出 (REAR): -(500 + 前悬长 - offset_x·sin|θ|)

  负号含义: X' 负方向 = 车位外侧 (驶出方向)
  后悬/前悬: 车的后/前保险杠到后轴/前轴的距离
  sin|θ|: 车不正对车位时, 悬长在 X' 方向的有效分量修正

后续追踪:
  end_pos.x (上一帧) + offset_x·sin|θ|
  → 沿用上一帧 X' 距离, 加一个 FSD 入侵修正
```

### 段 ⑥ else 分支：叠加 obj2_pt 偏置（637–666）

非直出模式走到这里，已经算好 `default_end_pos_x` / `default_end_pos_y`。这一段把"**车位外侧边界点 obj2_pt**"也纳入修正：

```c
end_pos_local = (obj2_pt 转局部)
if (is_obj2_exist || is_slot_at_right)
    end_pos_local.x = 0;          // 把 obj2_pt 的 X' 强制清零

if (is_slot_at_right == FALSE)
    default_end_pos_x = -default_end_pos_x + end_pos_local.x;
else
    default_end_pos_x =  default_end_pos_x + end_pos_local.x;

final_end_pos_world.x = default_end_pos_x;
final_end_pos_world.y = default_end_pos_y;
final_end_pos_world = 反变换到世界系
EndPos.CarAng = anchor_ang   // 直接跟车位长边
```

**几何含义**：
- `obj2_pt` 是车位**外侧顶点**（车位远端边界点）
- 如果 `is_obj2_exist`（obj2 边线检测到）或车位在右侧，把它的 X' 强制归零 → 终点 X' 不受 obj2_pt 影响
- 否则按"车位在左/右"决定 X' 的叠加方向（左右对称翻转）
- `EndPos.CarAng = anchor_ang`：非直出模式下终点车头严格跟车位长边

### 段 ⑦ 返回（667–668）

```c
return final_end_pos_world;
```

**异常返回路径**（直出垂直线）：
```c
final_end_pos_world.x = 0xff;     // 哨兵值
APAMAP_Setfailcause(101);
return final_end_pos_world;        // 提前 return，不走到主返回
```

---

## 五、`is_end_pos_initialized` 静态标志位的生命周期

```
                      ┌─────────────────────────────┐
                      │  函数首次被调用              │
                      │  静态变量 = TRUE            │
                      └──────────────┬──────────────┘
                                     │
            ┌────────────────────────┼────────────────────────┐
            │                        │                        │
            ▼                        ▼                        ▼
   ┌────────────────┐      ┌──────────────────┐    ┌─────────────────┐
   │ APAstate<=3    │      │ 首次进入计算分支 │    │ 首次进入直出分支│
   │ 状态切换       │      │ → 用查表默认值   │    │ → 用物理参数算  │
   │ → 重置为 TRUE  │      │ → 置 FALSE       │    │ → 置 FALSE      │
   └────────────────┘      └────────┬─────────┘    └────────┬────────┘
                                    │                       │
                                    ▼                       ▼
                            ┌────────────────┐     ┌────────────────┐
                            │ 后续帧:        │     │ 后续帧:        │
                            │ 跟当前车位置   │     │ 跟上一帧终点   │
                            └────────────────┘     └────────────────┘
```

> ⚠️ **隐患**：`static` 修饰意味着这个标志位是**跨调用持久**的。一旦 APA 状态机没有正确触发 ② 段的重置条件，函数可能一直走"后续帧"分支，沿用错误的旧值。

---

## 六、关键几何/数学约定速查

| 项 | 值/约定 |
|----|---------|
| X' 正方向 | 车位长边方向（`anchor_ang`） |
| X' 负方向 | 车位外侧（驶出方向） |
| Y' 正方向 | 车位外侧，垂直于 X' |
| World → Local | `AlgCom_GetPtCoordinateWithGivenOPtAndAngInOldCorSys(p, 0, ang, pt)` |
| Local → World | `AlgCom_PointPosWithAngAndCenterPt(p, ang, pt)` |
| 角度归一化 | `AlgCom_AngNormalized(&x)` 把 x 折到 (-π, π] |
| `STRAIGHT_OUT_DEFAULT_END_X_OFFSET_MM` | 500 mm（直出默认 X' 偏移） |
| 异常哨兵 | `0xff`（X 坐标）、错误码 `101`（直出垂直线） |

---

## 七、可挑出来的重构建议

1. **`final_end_pos_world` 命名误导** — 在 ⑤ 段里被用作"局部坐标 (X',Y')"，到 ⑥ 段才被覆写为世界坐标。建议中间引入 `end_pos_local_in_anchor_frame` 之类命名，避免歧义。
2. **`static` 标志位有副作用** — `is_end_pos_initialized` 在多实例/多线程或单元测试时会污染状态。建议改为函数入参或 caller 维护。
3. **FSD 决策表可抽出** — 段 ③ 是 4×3 的决策矩阵，可以抽成查表函数 `ComputeFsdInvasionCompensation(...)`，主函数瘦身。
4. **魔法数字散落** — `950`, `1100`, `2000`, `3100`, `5000`, `4500`, `2000` 等经验值应集中到宏定义区（参考 Gemini 风格里 `STRAIGHT_OUT_DEFAULT_END_X_OFFSET_MM` 的做法）。
5. **直出 vs 非直出可拆函数** — `park_out_mode` 的两种分支逻辑差异大，建议拆成 `SetStraightExitEndPos()` 和 `SetCurvedExitEndPos()` 两个子函数，主函数只做模式分发。

---

## 八、参考文件

- **原函数**：`/media/disk/2818_proj/local_parkout/src/pnc_map/park_out_pnc_map.cpp` 433-668 行
- **重构对照**：`/media/disk/2818_proj/local_parkout/src/pnc_map/SetEndCarPos_refactored.cpp`
- **Gemini 风格规范**：`~/.claude/projects/-media-disk-2818-proj-dev-Loc/memory/gemini_refactor_style.md`
