---
name: tsd-header-decoder
description: Use when encountering TSD-Header encoded files (files with %TSD-Header-###% prefix) that need to be decoded/read. Triggers: binary/garbled content in source files, files that appear intercepted or encoded.
---

# TSD-Header Decoder

## Overview

Decode files with TSD-Header encoding format. These files appear as binary/garbled when read directly due to being intercepted and encoded with a custom format.

## What is TSD-Header Encoding?

Files with this format have:
- Prefix: `%TSD-Header-###%`
- May contain bzip2 compressed data OR be plain UTF-8 text
- Actual content varies by file

## How to Decode

### Step 1: Check File Header
```bash
head -c 20 /path/to/file.cpp | od -c
```

If you see `%TSD-Header-###%bz`, the file contains bzip2 compressed data.
If you see `%TSD-Header-###%` followed by readable text (like `APACoordinateDataCalFloatType`), the file is likely UTF-8 text.

### Step 2a: For UTF-8 Encoded Files (most common)
```python
python3 -c "
with open('/path/to/file.cpp', 'r', encoding='utf-8', errors='replace') as f:
    content = f.read()
print(content)
"
```

### Step 2b: For bzip2 Compressed Files (rare)
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

## Encoding Detection Checklist

| Indicator | Meaning |
|-----------|---------|
| `%TSD-Header-###%` in od -c output | File is TSD encoded |
| Garbled text with `%` signs | Likely TSD encoded |
| `BZ` magic bytes after header | Contains bzip2 compressed data |
| `APACoordinateDataCalFloatType` or similar readable text after header | Plain UTF-8 text |
| Chinese characters appearing as `\xe` etc. | Encoding issue, may be UTF-8 needing direct read |

## Quick Decode Function

```python
#!/usr/bin/env python3
"""TSD-Header encoded file decoder"""

import sys

def decode_tsd_file(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()

    # Check for bzip2 magic
    bz2_pos = data.find(b'BZ')
    if bz2_pos > 0:
        import bz2
        decompressed = bz2.decompress(data[bz2_pos:])
        return decompressed.decode('utf-8', errors='replace')

    # Try UTF-8 direct
    return data.decode('utf-8', errors='replace')

if __name__ == '__main__':
    if len(sys.argv) > 1:
        print(decode_tsd_file(sys.argv[1]))
```

## Common Mistakes

| Mistake | Correction |
|---------|-----------|
| Reading with `cat` or `less` | These show binary, use Python with UTF-8 |
| Assuming all TSD files are bzip2 | Try UTF-8 direct read first |
| Using `od -c` alone | Good for detection, but need Python to decode |
| Giving up if bzip2 decompression fails | Try direct UTF-8 read |

## Example Sessions

**Case 1: Plain UTF-8 file**
```
$ head -c 20 gemini_refacotr.cpp | od -c
0000000   %   T   S   D   -   H   e   a   d   e   r   -   #   #   #   %

$ python3 -c "print(open('gemini_refacotr.cpp','r',encoding='utf-8').read()[:100])"
// ==========================================
// 局部物理阈值宏定义（消除所有"魔法数字"）...
```

**Case 2: Garbled UTF-8 file (use errors='replace')**
```
$ python3 -c "
with open('somefile.h', 'r', encoding='utf-8', errors='replace') as f:
    print(f.read()[:500])
"
```

## When to Use This Skill

- File displays as binary/garbled when `cat` or Read tool is used
- `od -c` shows `%TSD-Header-###%` prefix
- File has `.cpp` or `.h` extension but contains unusual content
- User mentions file is "intercepted" ("被拦截了")