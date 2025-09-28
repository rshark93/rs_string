# 🦾 rs_string — Modern Strings for C

> **C has no string type. We fix that.**  
> A safe, fast, feature-rich string library — header-only, pure C11.

---

## Why?

C developers suffer daily with `char*`:  
- `strlen`, `strcpy`, `malloc/free`,  
- buffer overflows,  
- null terminators,  
- clumsy APIs.

**rs_string** brings into C what C++, Rust, Go, Python already have:

- ✅ Length, capacity, offset — always stored  
- ✅ Small String Optimization (SSO)  
- ✅ Copy-On-Write + refcount  
- ✅ `append`, `replace`, `split`, `trim`, `starts_with`, `ends_with`  
- ✅ Fluent API (`RS(&s)->trim()->append(...)`)  
- ✅ UTF-8/16/32 helpers  
- ✅ Thread-safe mode with atomic refcount + `rs_string_ts` wrapper  
- ✅ Header-only, portable C11, tested on GCC/Clang/MSVC

---

## Example

### Before (raw `char*`)
```c
char buf[256];
strcpy(buf, " Hello ");
buf[strlen(buf)-1] = '\0';
strcat(buf, " world!");
printf("[%s] len=%zu\n", buf, strlen(buf));
```

### After (rs_string)
```c
#include "rs_string.h"

rs_string s = rs_string_from_val(" Hello ");
rs_string_trim(&s);
rs_string_append(&s, rs_sv_from_cstr(" world!"));
rs_string_printf(&s, "[%s] len=%zu", rs_string_cstr(&s), rs_string_len(&s));
puts(rs_string_cstr(&s));
rs_string_free(&s);
```

### Fluent sugar
```c
#define RS_ENABLE_FLUENT 1
#include "rs_string_fluent.h"

rs_string s = rs_string_from_val(" Hello ");
RS(&s)->trim()->append(rs_sv_from_cstr(" world!"))->printf_("[%s]", RS(&s)->cstr());
puts(RS(&s)->cstr());
RS(&s)->free_();
```

---

## Thread-Safety

- By default, `rs_string` is single-threaded.  
- For safe COW in multithreaded environments:  
  ```c
  #define RS_ATOMIC_REFCOUNT 1
  #include "rs_string.h"
  ```
- For a “coarse-grained” thread-safe API:  
  ```c
  #include "rs_string_ts.h"

  rs_string_ts t; rs_string_ts_init(&t);
  rs_string_ts_append(&t, rs_sv_from_cstr("hi"));
  size_t n = rs_string_ts_len(&t);
  rs_string_ts_free(&t);
  ```

---

## Comparison

| Feature              | C (`char*`)         | C++ `std::string` | Rust `String` | Python `str` | **rs_string** |
|----------------------|---------------------|------------------|---------------|--------------|---------------|
| Length stored        | ❌ `strlen` O(n)    | ✅ O(1)          | ✅ O(1)       | ✅ O(1)      | ✅ O(1)       |
| Capacity management  | ❌ manual           | ✅               | ✅            | N/A          | ✅            |
| Copy-On-Write        | ❌                 | ⚠️ (removed)     | ❌            | N/A          | ✅ optional   |
| Small String Opt.    | ❌                 | ✅ impl-dependent| ✅ (inline)   | N/A          | ✅            |
| UTF helpers          | ❌                 | partial          | ✅ full       | ✅ full      | ✅ basic      |
| Thread-safe mode     | ❌                 | partial          | ✅            | ✅           | ✅ wrapper    |
| Fluent API           | ❌                 | ❌               | ❌            | N/A          | ✅ optional   |

---

## Installation

### Conan
```bash
conan install rs_string/0.1.0@ --build=missing
```

### vcpkg
```bash
vcpkg install rs-string
```

Or just drop `rs_string.h` (and optional `rs_string_fluent.h`, `rs_string_ts.h`) into your project.

---

## Roadmap

- Regex-like API (`replace_regex`, `match`)  
- JSON / XML helpers  
- Arena allocator integration  
- WASM build & benchmarks  

---

## License

MIT. Free for everyone.  
