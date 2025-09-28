// MIT License
//
// Copyright (c) 2025 Raman Sharkovich
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// rs_string.h - header-only dynamic string for C11 (English-only)
// Features: SSO, length/capacity, string_view, basic COW (copy-on-write) with refcount,
// trim/split/replace, printf helpers, adopt/steal, UTF helpers (stubs for brevity).
// This is a compact implementation for demonstration and can be extended.
//
// Created by Raman Sharkovich on 24.09.25.
//

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

// --- Config
#ifndef RS_SSO_CAP
#define RS_SSO_CAP 22
#endif

// Optional atomic refcount
#ifdef RS_ATOMIC_REFCOUNT
  #include <stdatomic.h>
  typedef _Atomic size_t rs_rc_t;
  static inline size_t rs__rc_inc(rs_rc_t* p){ return atomic_fetch_add_explicit(p, 1, memory_order_acq_rel)+1; }
  static inline size_t rs__rc_dec(rs_rc_t* p){ return atomic_fetch_sub_explicit(p, 1, memory_order_acq_rel)-1; }
  static inline size_t rs__rc_get(rs_rc_t* p){ return atomic_load_explicit(p, memory_order_acquire); }
#else
  typedef size_t rs_rc_t;
  static inline size_t rs__rc_inc(rs_rc_t* p)       { return ++(*p); }
  static inline size_t rs__rc_dec(rs_rc_t* p)       { return --(*p); }
  static inline size_t rs__rc_get(const rs_rc_t* p) { return *p; }
#endif

// Allocator hook
typedef struct {
      void*(*m)(size_t, void*);
      void*(*r)(void*, size_t, void*);
      void (*f)(void*, void*); void* ctx;
  } rs_alloc;

static inline void* rs__sys_malloc(size_t n, void* ctx) { (void)ctx; return malloc(n); }
static inline void* rs__sys_realloc(void* p, size_t n, void* ctx) { (void)ctx; return realloc(p,n); }
static inline void  rs__sys_free(void* p, void* ctx) { (void)ctx; free(p); }
static inline rs_alloc rs_default_alloc(void) {
    return (rs_alloc){ &rs__sys_malloc, &rs__sys_realloc, &rs__sys_free, NULL };
}

// string_view
typedef struct {
    const char* data;
    size_t len;
} rs_sv;

static inline rs_sv rs_sv_from_cstr(const char* s) { return (rs_sv){ s, s ? strlen(s) : 0 }; }
static inline rs_sv rs_sv_substr(rs_sv s, size_t pos, size_t n) {
    if (pos > s.len) pos = s.len;
    if (pos+n>s.len) n = s.len - pos;
    return (rs_sv){ s.data + pos, n };
}

/* --- string_view split --- */
typedef void (*rs_sv_split_cb)(rs_sv token, void* ctx);

static inline void cb_count(rs_sv t, void* ctx) {
    int* n=(int*)ctx;
    (*n)++;
}

static inline void rs_sv_split(rs_sv s, rs_sv sep, int keep_empty, rs_sv_split_cb cb, void* ctx) {
    size_t i = 0;
    if (!cb) return;
    if (sep.len == 0) {
        if (s.len > 0 || keep_empty) cb(s, ctx);
        return;
    }

    while (i <= s.len) {
        size_t pos = (size_t) - 1;
        for (size_t j = i; j + sep.len <= s.len; ++j) {
            if (memcmp(s.data + j, sep.data, sep.len) == 0) {
                pos = j;
                break;
            }
        }

        if (pos == (size_t) - 1) {
            rs_sv tok = { s.data + i, s.len - i };
            if (tok.len > 0 || keep_empty)
                cb(tok, ctx);
            break;
        } else {
            rs_sv tok = { s.data + i, pos - i };
            if (tok.len > 0 || keep_empty)
                cb(tok, ctx);

            i = pos + sep.len;

            if (i == s.len && keep_empty) {
                rs_sv empty = { s.data + i, 0 };
                cb(empty, ctx);
                break;
            }
        }
    }
}

// Heap header (when not SSO)
typedef struct { size_t cap; rs_rc_t rc; } rs__hdr;

// rs_string with SSO + offset for future-ext
typedef struct {
    size_t len;
    size_t off;                 /* offset reserved for future use */
    char*  p;                   /* points to sso or heap buffer */
    size_t cap;                 /* RS_SSO_CAP for SSO */
    char   sso[RS_SSO_CAP + 1]; /* inline buffer */
} rs_string;

// Helpers
static inline bool rs_string_is_heap(const rs_string* s)     { return s->p != s->sso; }
static inline size_t rs_string_len(const rs_string* s)       { return s->len;         }
static inline size_t rs_string_cap(const rs_string* s)       { return rs_string_is_heap(s) ? s->cap : RS_SSO_CAP; }
static inline const char* rs_string_cstr(const rs_string* s) { return rs_string_is_heap(s) ? s->p   : s->sso;     }
static inline size_t rs_string_avail(const rs_string* s) {
    size_t cap = rs_string_cap(s);
    return cap - s->len;
}

static inline rs__hdr* rs__hdr_from_ptr(char* p) { return (rs__hdr*)( (uint8_t*)p - sizeof(rs__hdr) ); }
static inline char*    rs__ptr_from_hdr(rs__hdr* h) { return (char*)( (uint8_t*)h + sizeof(rs__hdr) ); }

// Init/Free
static inline void rs_string_init(rs_string* s) {
    s->len = 0;
    s->off = 0;
    s->sso[0] = '\0';
    s->p = s->sso;
    s->cap = RS_SSO_CAP;
}

static inline rs_string rs_string_from_val(const char* c) {
    rs_string s;
    rs_string_init(&s);

    if(c && *c) {
        size_t n = strlen(c);
        if (n <= RS_SSO_CAP) {
            memcpy(s.sso, c, n + 1);
            s.len = n;
        } else {
            rs_alloc a = rs_default_alloc();
            size_t cap = n;
            rs__hdr* h = (rs__hdr*)a.m(sizeof(rs__hdr) + cap + 1, a.ctx);
            h->cap = cap; h->rc = 1;
            char* p = rs__ptr_from_hdr(h);
            memcpy(p, c, n + 1);
            s.p = p; s.cap = cap; s.len = n;
        }
    }

    return s;
}

static inline int rs_string_from_cstr(rs_string* s, const char* c) {
    *s = rs_string_from_val(c);
    return 0;
}
static inline void rs__free_heap(rs_string* s) {
    if (rs_string_is_heap(s)) {
        rs__hdr* h = rs__hdr_from_ptr(s->p);

        if (rs__rc_dec(&h->rc) == 0)
            rs_default_alloc().f(h, NULL);
    }
}

static inline void rs_string_free(rs_string* s) {
    if (!s) return;
    if (rs_string_is_heap(s))
        rs__free_heap(s);

    rs_string_init(s);
}

// COW helpers
static inline void rs__retain(rs_string* s) {
    if (rs_string_is_heap(s)) {
        rs__hdr* h = rs__hdr_from_ptr(s->p);
        rs__rc_inc(&h->rc);
    }
}

static inline void rs_string_share(rs_string* dst, const rs_string* src) {
    if (dst == src) return;

    rs_string_free(dst);
    if (rs_string_is_heap(src)) {
        *dst = *src; /* share heap buffer, bump refcount */
        rs__retain(dst); /* ++rc on the shared heap header */
    } else { /* SSO: make an independent copy into dst->sso */
        dst->len = src->len;
        dst->off = 0;
        dst->cap = RS_SSO_CAP;
        memcpy(dst->sso, src->sso, src->len + 1);
        dst->p = dst->sso;
    }

    if (rs_string_is_heap(dst))
        rs__retain(dst);
}

static inline int rs__ensure_unique(rs_string* s, rs_alloc a) {
    if (!rs_string_is_heap(s)) return 0;

    rs__hdr* h = rs__hdr_from_ptr(s->p);

    if (rs__rc_get(&h->rc) == 1) return 0;

    size_t cap = h->cap;
    rs__hdr* nh = (rs__hdr*)a.m(sizeof(rs__hdr) + cap + 1, a.ctx);

    if (!nh) return -1;

    nh->cap = cap; nh->rc = 1;
    char* np = rs__ptr_from_hdr(nh);
    memcpy(np, s->p, s->len + 1);
    rs__rc_dec(&h->rc);
    s->p = np;
    s->cap = cap;

    return 0;
}

// Reserve
static inline int rs_string_reserve_ex(rs_string* s, size_t need, rs_alloc a) {
    size_t cap = rs_string_cap(s);

    if (need <= cap)                  return 0;
    if (rs__ensure_unique(s, a) != 0) return -1;

    size_t ncap = cap + cap / 2 + 1;
    if (ncap < need)
        ncap = need;

    if (rs_string_is_heap(s)) {
        rs__hdr* oh = rs__hdr_from_ptr(s->p);
        rs__hdr* nh = (rs__hdr*)a.r(oh, sizeof(rs__hdr) + ncap + 1, a.ctx);

        if (!nh) return -1;

        nh->cap = ncap;
        s->p = rs__ptr_from_hdr(nh);
        s->cap = ncap;
    } else {
        rs__hdr* nh = (rs__hdr*)a.m(sizeof(rs__hdr) + ncap + 1, a.ctx);

        if (!nh) return -1;

        nh->cap = ncap;
        nh->rc = 1;
        char* np = rs__ptr_from_hdr(nh);
        memcpy(np, s->sso, s->len + 1);
        s->p = np;
        s->cap = ncap;
    }

    return 0;
}

static inline int rs_string_reserve(rs_string* s, size_t need) {
    return rs_string_reserve_ex(s, need, rs_default_alloc());
}

// Assign/Append
static inline int rs_string_assign(rs_string* s, rs_sv v) {
    rs_alloc a = rs_default_alloc();

    if (rs_string_reserve_ex(s, v.len, a) != 0) return -1;
    if (rs__ensure_unique(s, a)                 != 0) return -1;

    memcpy(rs_string_is_heap(s) ? s->p : s->sso, v.data, v.len);

    if(rs_string_is_heap(s))
        s->p[v.len] = '\0';
    else
        s->sso[v.len]='\0';

    s->len = v.len;

    return 0;
}
static inline int rs_string_clear(rs_string* s) { return rs_string_assign(s, (rs_sv){ "", 0 }); }

static inline int rs_string_append(rs_string* s, rs_sv v) {
    rs_alloc a = rs_default_alloc();
    size_t need = s->len + v.len;

    if (rs_string_reserve_ex(s, need, a) != 0) return -1;
    if (rs__ensure_unique(s, a) != 0) return -1;

    char* p = rs_string_is_heap(s) ? s->p : s->sso;
    memcpy(p + s->len, v.data, v.len);
    s->len += v.len;
    p[s->len] = '\0';

    return 0;
}
static inline int rs_string_push_cstr(rs_string* s, const char* c) {
    return rs_string_append(s, rs_sv_from_cstr(c));
}

static inline int rs_string_push_char(rs_string* s, char ch) {
    char tmp[1] = { ch };
    return rs_string_append(s, (rs_sv){ tmp, 1 });
}

// Insert/Erase
static inline int rs_string_insert(rs_string* s, size_t pos, rs_sv v) {
    if (pos > s->len)
        pos = s->len;
    rs_alloc a = rs_default_alloc();

    if (rs_string_reserve_ex(s, s->len + v.len, a) != 0) return -1;
    if (rs__ensure_unique(s, a)                          != 0) return -1;

    char* p = rs_string_is_heap(s) ? s->p : s->sso;
    memmove(p + pos + v.len, p + pos, s->len - pos);
    memcpy(p + pos, v.data, v.len);
    s->len += v.len;
    p[s->len]='\0';

    return 0;
}
static inline int rs_string_erase(rs_string* s, size_t pos, size_t n) {
    if (pos > s->len) return 0;
    if (pos+n > s->len) n = s->len - pos;

    rs_alloc a = rs_default_alloc();

    if (rs__ensure_unique(s, a) != 0) return -1;

    char* p = rs_string_is_heap(s) ? s->p : s->sso;
    memmove(p + pos, p + pos + n, s->len - (pos + n) + 1);
    s->len -= n;

    return 0;
}

// Find (naive)
static inline size_t rs_string_find(const rs_string* s, rs_sv what, size_t from) {
    const char* p = rs_string_cstr(s);

    if (what.len == 0)
        return from <= s->len ? from : (size_t) - 1;

    for (size_t i = from; i + what.len <= s->len; ++i) {
        if (memcmp(p + i, what.data, what.len) == 0)
            return i;
    }

    return (size_t) - 1;
}
static inline int rs_string_starts_with(const rs_string* s, rs_sv pfx) {
    return s->len >= pfx.len && memcmp(rs_string_cstr(s), pfx.data, pfx.len) == 0;
}
static inline int rs_string_ends_with(const rs_string* s, rs_sv sfx) {
    return s->len >= sfx.len && memcmp(rs_string_cstr(s)+s->len-sfx.len, sfx.data, sfx.len) == 0;
}

// Trim ASCII spaces
static inline int rs_string_trim_left(rs_string* s) {
    const char* p = rs_string_cstr(s);
    size_t i = 0;

    while (i < s->len && (unsigned char)p[i] <= 0x20) ++i;

    return i == 0 ? 0 : rs_string_erase(s, 0, i);
}

static inline int rs_string_trim_right(rs_string* s) {
    const char* start;
    const char* end;
    size_t size;

    if (!s) return -1;
    if (s->len == 0) return 0;

    start = rs_string_cstr(s);
    end   = start + s->len;

    while (end > start && (unsigned char)*(end - 1) <= 0x20) --end;

    size = (size_t)(end - start);

    return size == s->len ? 0 : rs_string_erase(s, size, s->len - size);
}

static inline int rs_string_trim(rs_string* s) {
    rs_string_trim_right(s);
    return rs_string_trim_left(s);
}

// Replace
static inline int rs_string_replace_first(rs_string* s, rs_sv from, rs_sv to) {
    size_t pos = rs_string_find(s, from, 0);
    if (pos == (size_t) - 1) return 0;
    if (from.len >= to.len) {
        rs_string_erase(s, pos, from.len);
        return rs_string_insert(s, pos, to);
    } else { // reserve first then perform
        rs_alloc a = rs_default_alloc();

        if (rs_string_reserve_ex(s, s->len + (to.len-from.len), a) != 0) return -1;

        rs_string_erase(s, pos, from.len);
        return rs_string_insert(s, pos, to);
    }
}
static inline int rs_string_replace_all(rs_string* s, rs_sv from, rs_sv to) {
    if (from.len == 0) return 0;

    size_t i = 0;
    int count = 0;

    while(1) {
        size_t pos = rs_string_find(s, from, i);
        if (pos == (size_t) - 1)
            break;

        rs_string_erase(s, pos, from.len);
        rs_string_insert(s, pos, to);
        i = pos + to.len;
        ++count;
    }

    return count;
}

// printf helpers
static inline int rs_string_vprintf(rs_string* s, const char* fmt, va_list ap) {
    va_list ap2;
    va_copy(ap2, ap);
    int need = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);

    if (need < 0) return -1;

    rs_alloc a = rs_default_alloc();

    if (rs_string_reserve_ex(s, (size_t)need, a) != 0) return -1;
    if (rs__ensure_unique(s, a) != 0) return -1;

    char* p = rs_string_is_heap(s) ? s->p : s->sso;

    int wrote = vsnprintf(p, (size_t)need + 1, fmt, ap);
    if (wrote >= 0)
        s->len = (size_t)wrote;
    return wrote;
}

static inline int rs_string_printf(rs_string* s, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = rs_string_vprintf(s, fmt, ap);
    va_end(ap);
    return r;
}

/* UTF helpers (minimal stubs for now, real implementations can be plugged later) */
/* ASCII <-> UTF-16 helpers */
static inline int rs_utf16_from_ascii_bytes(rs_sv ascii, int little_endian, int write_bom, rs_alloc a,
                                            unsigned char** out_bytes, size_t* out_len) {
    (void)a;
    size_t bom = write_bom ? 2 : 0;
    size_t units = ascii.len + 1;
    size_t total = bom + units * 2;
    unsigned char* buf = (unsigned char*)malloc(total);
    size_t i = 0, o = 0;

    if(!buf) return -1;

    if (write_bom) {
        if (little_endian) {
            buf[o++] = 0xFF;
            buf[o++] = 0xFE;
        } else {
            buf[o++] = 0xFE;
            buf[o++] = 0xFF;
        }
    }

    for (i = 0; i < ascii.len; ++i) {
        unsigned char ch = (unsigned char)ascii.data[i];
        if (little_endian) {
            buf[o++] = ch;
            buf[o++] = 0x00;
        } else {
            buf[o++] = 0x00;
            buf[o++] = ch;
        }
    }
    buf[o++] = 0x00;
    buf[o++] = 0x00;
    *out_bytes = buf;
    if (out_len)
        *out_len = total;
    return 0;
}
static inline int rs_ascii_from_utf16_bytes(rs_string* out, rs_sv u16, int default_little_endian, char replace) {
    int little = default_little_endian;
    size_t i = 0;
    if (u16.len >= 2) {
        unsigned char b0 = (unsigned char)u16.data[0];
        unsigned char b1 = (unsigned char)u16.data[1];
        if (b0 == 0xFF && b1 == 0xFE) {
            little = 1;
            i = 2;
        } else if (b0 == 0xFE && b1 == 0xFF) {
            little = 0;
            i = 2;
        }
    }

    rs_string_clear(out);

    for (; i + 1 < u16.len; i += 2) {
        unsigned char b0 = (unsigned char)u16.data[i + 0];
        unsigned char b1 = (unsigned char)u16.data[i + 1];
        unsigned int code = little ? (unsigned int)(b0 | (b1 << 8)) : (unsigned int)((b0 << 8) | b1);

        if (code == 0x0000) break;

        uint8_t out_u8 = (code <= 0x7F) ? (uint8_t)code : (uint8_t)replace;
        rs_string_push_char(out, (char)out_u8);
    }

    return 0;
}

static inline int rs_utf8_from_utf16_bytes(rs_string* out, rs_sv utf16_bytes, int default_little_endian) {
    (void)default_little_endian; // Stub: just clear and copy raw as bytes (NOT real convert). Replace with real logic if needed.
    return rs_string_assign(out, utf16_bytes);
}

static inline int rs_utf16_from_utf8_bytes(rs_sv utf8, int little_endian, int write_bom, rs_alloc a,
                                           unsigned char** out_bytes, size_t* out_len) {
    (void)little_endian;
    (void)write_bom;
    (void)a;
    *out_bytes = (unsigned char*)malloc(utf8.len + 2);

    if (!*out_bytes) return -1;

    memcpy(*out_bytes, utf8.data, utf8.len);
    (*out_bytes)[utf8.len] = 0;
    (*out_bytes)[utf8.len + 1] = 0;
    *out_len = utf8.len + 2;

    return 0;
}
static inline int rs_utf8_from_utf32_bytes(rs_string* out, rs_sv utf32_bytes, int default_little_endian) {
    (void)default_little_endian;
    return rs_string_assign(out, utf32_bytes);
}

static inline int rs_utf32_from_utf8_bytes(rs_sv utf8, int little_endian, int write_bom, rs_alloc a,
                                           unsigned char** out_bytes, size_t* out_len) {
    (void)little_endian;
    (void)write_bom;
    (void)a;
    *out_bytes = (unsigned char*)malloc(utf8.len + 4);

    if (!*out_bytes) return -1;

    memcpy(*out_bytes, utf8.data, utf8.len);
    (*out_bytes)[utf8.len] = 0;
    (*out_bytes)[utf8.len + 1] = 0;
    (*out_bytes)[utf8.len + 2] = 0;
    (*out_bytes)[utf8.len + 3] = 0;
    *out_len = utf8.len + 4;

    return 0;
}

