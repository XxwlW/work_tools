---
name: cpp-large-file-refactoring
description: Use when refactoring large C++ files (>500KB) with complex state machines and global variables, particularly parking out (APA) related modules with unclear type definitions and naming conventions
---

# C++ Large File Refactoring

## Overview

Refactor large, monolithic C++ files into well-structured, maintainable code with clear type definitions, consistent naming conventions, and organized sections.

**Core principle:** Preserve functionality while improving code organization and readability.

## When to Use

- Source files exceeding 500KB with tangled dependencies
- State machine enums scattered across files without clear structure
- Global variables used inconsistently (g_ prefix, Hungarian notation)
- Type definitions (struct/enum) mixed with implementation
- Comments and sections using inconsistent styles

**Typical symptoms:**
- Finding function boundaries is difficult
- State transitions unclear without tracing all usages
- Global variable values unpredictable across functions
- Section comments using different styles (// vs /**/)
- Variable names don't follow consistent patterns

## Pre-Refactoring Checklist

1. **Analyze current state:**
   - Count total lines and major sections
   - Identify all enum/struct type definitions
   - List all global variables (g_ prefix, extern declarations)
   - Map function call dependencies

2. **Create backup:**
   ```bash
   # Use mmap-based copy to handle large binary files
   python3 -c "
   import mmap, shutil
   from pathlib import Path
   src = Path('src.cpp')
   dst = Path('backup_src.cpp')
   with open(src, 'rb') as f_in, open(dst, 'wb') as f_out:
       with mmap.mmap(f_in.fileno(), 0, access=mmap.ACCESS_READ) as mm:
           f_out.write(mm[:])
   "
   ```

3. **Define target structure:**

```
Section Order (recommended):
1. License header / includes
2. Macro definitions (#ifndef, #define, #pragma)
3. Type definitions (enum, struct, class)
4. External declarations (extern variables)
5. Global variable definitions (with comments)
6. Static function declarations (optional)
7. Implementation functions (grouped by feature)
```

## Refactoring Patterns

### 1. State Machine Organization

**Before:** Scattered enum definitions
```cpp
enum { IDLE, RUNNING, ERROR }; // What state? For what?
static int state = 0;
```

**After:** Grouped with clear prefix and comments
```cpp
/**
 * Park Out State Machine
 * Controls navigation from parked position to exit point
 */
typedef enum {
    PARK_OUT_STATE_IDLE = 0,           // 初始空闲状态
    PARK_OUT_STATE_INIT,               // 初始化中
    PARK_OUT_STATE_PLANNING,           // 路径规划中
    PARK_OUT_STATE_EXECUTING,          // 执行中
    PARK_OUT_STATE_COMPLETE,           // 完成
    PARK_OUT_STATE_ERROR               // 错误状态
} park_out_state_e;

/**
 * Global status for park out module
 */
typedef struct {
    park_out_state_e state;
    uint8_t slot_type;
    uint32_t timestamp;
} park_out_status_t;

static park_out_status_t g_park_out_status = {0};
```

### 2. Macro Definition Standards

**Format:**
```cpp
/**
 * @name Slot Size Definitions
 * @brief Physical dimensions for parking slots
 */
///@{
#define PARK_OUT_SLOT_WIDTH_MIN     2.5   // 最小宽度(m)
#define PARK_OUT_SLOT_WIDTH_MAX     3.5   // 最大宽度(m)
#define PARK_OUT_SLOT_LENGTH_MIN    5.0   // 最小长度(m)
///@}
```

**Rules:**
- Group related macros with `@name` comments
- Use uppercase with underscore separator
- Add units in comments (m, ms, degree)
- Align values for readability

### 3. Variable Naming Conventions

**Naming对照表:**

| Old Pattern | New Pattern | Example |
|-------------|-------------|---------|
| m_nXXX | member_XXX | m_nCount → member_count |
| g_ | g_ global | g_status → g_park_out_status |
| bXXX | bool_XXX | bValid → bool_is_valid |
| pXXX | ptr_XXX | pData → ptr_data |
| nXXX | count_XXX | nSize → count_size |

**Global variable declaration pattern:**
```cpp
// Type definition first (in header or top)
// Then extern declaration if shared
// Then definition with initializer

/** @brief Park out slot information */
typedef struct {
    float width;
    float length;
    float angle;
} park_out_slot_info_t;

/** @brief Global slot info - accessible across modules */
extern park_out_slot_info_t g_park_out_slot_info;
```

### 4. Section Comment Style

**Use consistent style throughout:**

```cpp
/**
 * ===========================================================================
 * Section: Map Boundary Calculation
 * ===========================================================================
 */

// Or for subsections:
/================== Boundary Processing ==================
```

**NOT:**
```cpp
// Section 1: xxx     (inconsistent)
// ===========        (mixing styles)
// ### xxx ###        (non-standard)
```

### 5. Function Grouping

**Group by functionality:**
```cpp
// ===========================================================================
// Group: Slot Border Calculation
// ===========================================================================
static Point2d CalSlotBorderPtByFSD(const FSDInfo& fsd_info);
static Point2d CalSlotBorderPtByODMap(const ODMapInfo& od_map);

// ===========================================================================
// Group: Boundary Update
// ===========================================================================
static BOOLEAN UpDataMapBoundaryByPDCInfo(const PDCInfo& pdc);
static BOOLEAN UpDataMapBoundaryByUSSInfo(const USSInfo& uss);
```

## Implementation Steps

1. **Extract type definitions to top:**
   - Move all `enum`, `struct`, `typedef` to file beginning
   - Group by purpose (state, config, data)
   - Add documentation comments

2. **Define global variable structure:**
   - Create single struct for related globals
   - Use `extern` for cross-file access
   - Prefix with `g_` consistently

3. **Add section comments:**
   - Use standard format (`// ====...====`)
   - One blank line before, two after
   - Group related functions together

4. **Rename variables:**
   - Apply naming convention table
   - Use search-replace for consistency
   - Update all references

5. **Test after each change:**
   - Compile frequently
   - Test boundary conditions
   - Verify state machine transitions

## Common Mistakes

| Mistake | Fix |
|---------|-----|
| Renaming without updating all refs | Use IDE refactor or careful search-replace |
| Breaking state machine transitions | Map all state dependencies before changing |
| Removing "unused" globals | Check all extern references first |
| Inconsistent section style | Pick one style and apply globally |

## Quick Reference

**Section comment template:**
```cpp
// ===========================================================================
// Section: [Description]
// ===========================================================================
```

**Type definition template:**
```cpp
/**
 * @brief [Description]
 * [Detailed explanation if needed]
 */
typedef enum/struct {
    // members
} type_name_e/t;
```

**Global variable template:**
```cpp
/** @brief [Description] */
static type g_variable_name = init_value;
```

## File Size Thresholds

| Size | Strategy |
|------|----------|
| <100KB | May not need refactoring |
| 100-300KB | Consider splitting if multiple distinct modules |
| 300-500KB | Group functions by feature, add clear sections |
| >500KB | Strong recommendation for structural refactoring |

## Backup Verification

Always verify backup was created correctly:
```bash
md5sum original.cpp backup_original.cpp
diff original.cpp backup_original.cpp
```