---
name: park-out-refactor-workflow
description: Use when refactoring parking out (APA泊出) C++ code that may be TSD-Header encoded. Triggers: files with %TSD-Header-###% prefix, or request to refactor parking out code end-to-end.
---

# Park Out Refactor Workflow

## Overview

End-to-end workflow for refactoring parking out C++ code:
1. **Decode** TSD-Header encoded files first
2. **Refactor** using Gemini refactor style (or GPT refactor style)

This workflow combines two skills into one cohesive process.

## Workflow Steps

### Step 1: Decode TSD-Header Encoded Files

**Detection command:**
```bash
head -c 20 /path/to/file.cpp | od -c
```

**Decode method (UTF-8 plain text):**
```python
python3 -c "
with open('/path/to/file.cpp', 'r', encoding='utf-8', errors='replace') as f:
    content = f.read()
print(content)
"
```

**Decode method (bzip2 compressed):**
```python
python3 -c "
with open('/path/to/file.cpp', 'rb') as f:
    data = f.read()
bz2_pos = data.find(b'BZ')
if bz2_pos > 0:
    import bz2
    decompressed = bz2.decompress(data[bz2_pos:])
    print(decompressed.decode('utf-8', errors='replace'))
"
```

### Step 2: Apply Refactor Style

Choose the appropriate style based on project requirements:

#### Option A: GPT-Refactor Style (Modern C++)
Uses `constexpr` + `k` prefix for constants:
```c
/*--------------------------------------------------------------------------*/
/* 固定参数（经验标定值）                                                   */
/*--------------------------------------------------------------------------*/
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kStraightExitDefaultOffsetXMm = 500.0F;
constexpr APA_DISTANCE_CAL_FLOAT_TYPE kParallelExitDistanceYMm = 2000.0F;
```

Variable naming: `boundary_intrusion_offset_x_mm`, `is_slot_on_right_side`
Block separator: `/*--------------------------------------------------------------------------*/`

#### Option B: Gemini-Refactor Style (Traditional C++)
Uses `#define` macros:
```c
// ==========================================
// 局部物理阈值宏定义（消除所有"魔法数字"）
// ==========================================
#define STRAIGHT_OUT_DEFAULT_END_X_OFFSET_MM      500.0f
```

Variable naming: `current_offset_x`, `max_outward_offset_x`
Block separator: `// ========== N. Section Name ==========`

## Reference Files

| Style | File |
|-------|------|
| GPT-Refactor (constexpr) | `/media/disk/2818_proj/local_parkout/record_code/gpt_refactor.cpp` |
| Gemini-Refactor (#define) | `/media/disk/2818_proj/local_parkout/record_code_new/gemini_refacotr.cpp` |
| Full refactoring | `/media/disk/2818_proj/local_parkout/record_code_new/MapParkingOut_refactored.cpp` |

## Variable Naming Convention

| Old Pattern | GPT Style | Gemini Style |
|-------------|-----------|--------------|
| islotatright | is_slot_on_right_side | is_slot_data_at_right |
| is_end_pos_invaded | is_boundary_intrusion_detected | is_end_pos_invaded |
| anchor_pt | anchor_origin | anchor_pt |
| bEndCarPosInitFlag | is_end_position_initialized | is_end_pos_initialized |
| MaxOutOffsetX | (not in GPT example) | max_outward_offset_x |

## Global Structure

Updated `g_park_out_status` structure (with underscore suffix naming):
```c
park_out_status_t {
  park_out_state_t park_out_state_;           // 原 park_out_state
  park_out_slot_proerties_t park_out_slot_properties_;  // 原 park_out_slot_proerties_t
  park_out_env_interference_t park_out_env_interference_;  // 原 park_out_env_interference_t
  ...
}
```

## When to Use

- User provides TSD-Header encoded parking out code
- Request to refactor and the source may be encoded
- Files with `.cpp` extension showing binary content
- Explicit request for "泊出代码重构" with encoded files

## Skills Combined

1. **tsd-header-decoder**: Decode encoded files first
2. **cpp-park-out-refactoring**: Apply Gemini or GPT refactor style