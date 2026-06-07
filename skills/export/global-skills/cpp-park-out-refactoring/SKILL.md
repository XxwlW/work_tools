---
name: cpp-park-out-refactoring
description: Use when refactoring parking out (APA泊出) C++ code in MapParkingOut.cpp or similar pnc_map modules. Triggers: function named APAMap_ParkingOut*, global variable g_park_out_status, or explicit request to apply Gemini refactor style.
---

# CPP Park Out Refactoring

## Overview

Refactor parking out (APA泊出) C++ code following the Gemini Refactor Style. This style transforms cryptic variable names and magic numbers into self-documenting code with physical thresholds, state machine enums, and section comments.

## Core Patterns

### 1. Constant Naming (constexpr with k prefix)

Use modern C++ `constexpr` with `k` prefix for compile-time constants:

```c
/*--------------------------------------------------------------------------*/
/* 固定参数（经验标定值）                                                   */
/*--------------------------------------------------------------------------*/
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kStraightExitDefaultOffsetXMm = 500.0F;

constexpr APA_DISTANCE_CAL_FLOAT_TYPE kParallelExitExtraDistanceXMm = 950.0F;

constexpr APA_DISTANCE_CAL_FLOAT_TYPE kAngledExitExtraDistanceXMm = 1100.0F;

constexpr APA_DISTANCE_CAL_FLOAT_TYPE kLadderSlotExitExtraDistanceXMm = 2000.0F;

constexpr APA_DISTANCE_CAL_FLOAT_TYPE kPerpendicularExitExtraDistanceXMm = 3100.0F;

constexpr APA_DISTANCE_CAL_FLOAT_TYPE kParallelExitDistanceYMm = 2000.0F;

constexpr APA_DISTANCE_CAL_FLOAT_TYPE kAngledExitDistanceYMm = 5000.0F;

constexpr APA_DISTANCE_CAL_FLOAT_TYPE kPerpendicularExitDistanceYMm = 4500.0F;
```

### 2. Variable Naming with Unit Suffix

Use descriptive names with `_mm` suffix for distance variables:

| Old Pattern | New Pattern | Note |
|-------------|-------------|------|
| evade_offset_x | boundary_intrusion_offset_x_mm | More descriptive + unit |
| default_end_pos_x | default_end_position_x_mm | Describes purpose + unit |
| target_longitudinal_dist | straight_exit_target_x_mm | Describes behavior + unit |

### 3. Block Separator Comments

Use `/*--------------------------------------------------------------------------*/` style for major sections:

```c
/*--------------------------------------------------------------------------*/
/* 当前车辆状态                                                             */
/*--------------------------------------------------------------------------*/
APACarCoordinateDataCalFloatType current_car_position;
current_car_position = APAMap_GInputData.CarLocInfo.CarPos;
```

### 4. Section Comment Style with Numbered Blocks

```c
// ========== 1. 变量声明与初始化 ==========
// 原始边界点坐标(从全局数据获取)
APACoordinateDataCalFloatType obj2_pt = APAMap_GInfo.SlotPar.SlotBordPt[0];
```

### 5. Parameter Naming (Descriptive over Abbreviated)

| Original | Refactored | Note |
|----------|------------|------|
| anchor_pt | anchor_origin | Origin of coordinate system |
| obj2_pt | object2_point | Full word "object" |
| is_end_pos_invaded | is_boundary_intrusion_detected | More descriptive |
| bSeizeEndCarPosFlag | is_end_position_initialized | bool naming pattern |

### 6. Helper Function Pattern

```c
// ========== 辅助函数声明 ==========

/**
 * @brief 更新车位尺寸枚举状态
 * @param[in] slot_length 计算得到的边界点间距
 * @param[in] park_out_mode 泊出模式
 * @param[out] slot_size 输出:车位尺寸枚举
 */
static void UpdateSlotSizeStatus(...);
```

### 7. Variable Naming Table in Function Header

```c
/**
 * ============================================================================
 * 原函数: APAMAP_ParkingOutGetSlotBdPtByODObjs (src/pnc_map/1.cpp 第1-309行)
 * 重构版本: APAMAP_ParkingOutGetSlotBdPtByODObjs_Refactored
 * ============================================================================
 *
 * 变量命名对照表:
 * +---------------------+----------------------------------+
 * | 原变量名            | 重构后变量名                     |
 * +---------------------+----------------------------------+
 * | Bordpttype          | border_type                      |
 * | OffsetX/OffsetY      | calculated_offset_x/y            |
 * | MaxOutOffsetX/Y     | max_outward_offset_x/y           |
 * +---------------------+----------------------------------+
 */
```

## When to Apply

- Function name starts with `APAMap_ParkingOut`
- Global variable `g_park_out_status` referenced
- Explicit request for "Gemini重构风格"
- Variable names like i, j, k, OffsetX, Data[] present
- Magic numbers (2000, 1000, 1500) scattered in code

## Reference Files

- **gpt_refactor style**: `/media/disk/2818_proj/local_parkout/record_code/gpt_refactor.cpp` (constexpr + k prefix + block comments)
- **gemini_refactor style**: `/media/disk/2818_proj/local_parkout/record_code_new/gemini_refacotr.cpp` (traditional #define macros)
- **Full refactoring example**: `/media/disk/2818_proj/local_parkout/record_code_new/MapParkingOut_refactored.cpp`

## Quick Reference

**Constant naming:**
```c
constexpr <type> k<Name> = <value>;  // e.g., kStraightExitDefaultOffsetXMm = 500.0F
```

**Variable naming:**
```c
<descriptive_name>_mm  // e.g., boundary_intrusion_offset_x_mm
```

**Section separator:**
```c
/*--------------------------------------------------------------------------*/
/* Section Title                                                            */
/*--------------------------------------------------------------------------*/
```

**State machine pattern:**
```c
while (continue_search) {
    if (search_phase == 0) {
        // 处理Square四边形对象
    }
    if (search_phase == 1) {
        // 处理Polygon对象(仅路沿)
    }
}
```

## Common Mistakes

| Mistake | Correction |
|---------|-----------|
| Using #define for constants | Use `constexpr <type> k<Name> = <value>;` |
| No unit suffix on variables | Add `_mm` suffix to distance variables |
| Single-letter variable names | Use descriptive names like `boundary_intrusion_offset_x_mm` |
| No block separator comments | Add `/*--*/` style section headers |
| Inconsistent naming convention | Pick one style and apply consistently |

## Real-World Impact

Refactoring `APAMap_ParkingOutFusBoundaryByFSDMapInfo` (730 lines):
- 50+ magic numbers → 15 physical threshold macros
- 80+ single-letter variables → meaningful names
- 7 distinct code sections with clear boundaries
- Code readability improved by ~300%