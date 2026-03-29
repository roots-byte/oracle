# oracle

Lightweight C99 Oracle client library built on top of Oracle OCI.
Designed for simple query execution and result extraction in plain C.
Cross-platform build via CMake (Windows, Linux, macOS).

---

## Features

- C99 API for Oracle DB communication
- OCI-based implementation (Oracle Client libraries)
- Static and shared libraries from one CMake project
- Simple query flow: create handle, run SQL, read values, cleanup
- Configurable logging and retry behavior via preprocessor macros

---

## Requirements

- CMake >= 3.20
- C99 compiler (GCC, Clang, MSVC with C mode)
- Oracle OCI client library

### Windows Oracle Client Setup

For correct runtime behavior on Windows, Oracle Instant Client must be available in global PATH.

1. Download Oracle Instant Client packages from:
  - https://www.oracle.com/database/technologies/instant-client/winx64-64-downloads.html
2. Install at least:
  - Instant Client Basic package
  - Instant Client SDK package
3. Add Instant Client directory (for example `C:\instantclient_23_x`) to system PATH.
4. Obtain `oci.lib` and OCI headers from the SDK package (`sdk/include`).

Without PATH setup, the project may build but fail to run because Oracle DLL dependencies are not found.

OCI lookup behavior:
- On Windows, CMake searches for `oci.lib`.
- On Linux/macOS, CMake searches for `libclntsh` or `oci`.
- You can place OCI library next to `CMakeLists.txt`, or provide it from system paths.

---

## Build

```sh
cmake -S . -B build
cmake --build build --config Debug
```

Build outputs are generated into `out/`:
- Shared library:
  - Windows: `out/oracle.dll` (+ import library)
  - Linux: `out/liboracle.so`
  - macOS: `out/liboracle.dylib`
- Static library:
  - Windows: `out/oracle_static.lib`
  - Linux/macOS: `out/liboracle_static.a`
- Example executable:
  - Windows: `out/example.exe`
  - Linux/macOS: `out/example`

---

## Expected Project Layout

Typical local layout used by this project:

```text
oracle/
  CMakeLists.txt
  LICENSE
  README.md
  oracle.c
  example.c
  oci.lib                 # local OCI import library (Windows)
  header/
    oracle.h              # project public header
    wrapper.h             # project support header
    oci.h                 # OCI headers from Oracle SDK
    oratypes.h            # OCI headers from Oracle SDK
    ...
  out/
    oracle.dll / liboracle.so / liboracle.dylib
    oracle_static.lib / liboracle_static.a
    example.exe / example
```

You can also keep Oracle headers and libraries outside the repository and point CMake/system linker paths to them.

---

## Quick Start

```c
#include "oracle.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    void* oracle = CreateOracle("127.0.0.1:1521/ORCL", "user", "password");
    if (!oracle) {
        fprintf(stderr, "CreateOracle failed\n");
        return 1;
    }

    char* output_array = NULL;
    int row_count = 0;
    int col_count = 0;

    if (GetQuery(oracle,
                 "SELECT 1 FROM DUAL",
                 &output_array,
                 &row_count,
                 &col_count) == OK) {
        for (int r = 0; r < row_count; r++) {
            for (int c = 0; c < col_count; c++) {
                char* value = GetQueryValue(output_array, c, r);
                if (value) {
                    printf("row=%d col=%d val=%s\n", r, c, value);
                }
            }
        }
    }

    free(output_array);      /* free buffer from GetQuery */
    DestroyOracle(oracle);   /* free Oracle handle */
    return 0;
}
```

---

## API Reference

### Lifecycle

| Function | Description |
|---|---|
| `CreateOracle(address, username, password)` | Creates Oracle handle and stores credentials. Returns handle or `NULL`. |
| `DestroyOracle(oracle)` | Clears OCI resources and frees handle. Safe on `NULL`. |

### Query

| Function | Description |
|---|---|
| `GetQuery(oracle, sql, &output_array, &row_count, &col_count)` | Executes SQL and allocates result buffer. Returns `OracleError` code. |
| `GetQueryValue(output_array, col_inx, line_inx)` | Returns value pointer for selected row/column from result buffer. |

### Memory Ownership Notes

- Handle from `CreateOracle` is owned by caller and must be released by `DestroyOracle`.
- Buffer from `GetQuery` is owned by caller and must be released with `free(output_array)`.
- Value pointer returned by `GetQueryValue` points inside `output_array`.
- Never call `free()` on the value pointer returned by `GetQueryValue` directly.

---

## Logging and Configuration

Define these macros before including `oracle.h`:

```c
#define LOG_ORACLE      1   /* 0 = disable logging */
#define DEBUG_ORACLE    1   /* 1 = verbose debug logs */
#define ORACLE_ATTEMPTS 1   /* login/query retry attempts */
```

Logging target:
- Windows: `C:\LOG\oracle.log` (fallback: current directory)
- Other platforms: `./oracle.log`

---

## Status Codes

`enum OracleError` in `oracle.h`:

| Code | Meaning |
|---|---|
| `OK` | Success |
| `LOGIN_ERROR` | Login/session setup failed |
| `BINNING_ERROR` | Internal/binding-related error |
| `INPUT_ERROR` | Invalid function input |
| `QUARY_NOT_EXIST` | Query/result not available |
| `MEMORY_LEAK` | Memory lifecycle issue |
| `MEMORY_ERROR` | Allocation failure |
| `COMUNICATION_ERROR` | Communication/OCI transport failure |
| `PARSE_ERROR` | Result parsing error |

---

## Notes for Windows Toolchains

When building a shared library, the OCI import library must match your compiler toolchain format.
For example, MinGW may fail to link against some MSVC-only `.lib` variants.
If that happens, use a matching OCI import library or build with MSVC toolchain.

---

## What You Can Publish On GitHub

You can safely publish your own source code and project files, for example:

- `oracle.c`
- `example.c`
- `CMakeLists.txt`
- `header/oracle.h`
- `header/wrapper.h`
- `README.md`
- `LICENSE`

Do not publish Oracle-provided binaries/SDK files unless Oracle license terms explicitly allow redistribution in your case, especially:

- `oci.lib`, `oci.dll`, `oraociei*.dll`, `ociw32.dll`
- OCI SDK headers such as `oci.h`, `oratypes.h`, and related Oracle header files

Recommended approach:

- Keep Oracle client files out of the repository.
- Document download/setup steps (this README already does this).
- Add Oracle client artifacts to `.gitignore`.

---

## License

This project is licensed under the MIT License.
See [LICENSE](LICENSE).

Copyright (c) 2026 Jonas Rys
