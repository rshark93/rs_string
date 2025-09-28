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
#ifndef RS_ENABLE_FLUENT
#define RS_ENABLE_FLUENT 0
#endif
#if RS_ENABLE_FLUENT

#include "rs_string.h"
#include <stdarg.h>

#ifndef RSF_USE_THREAD_LOCAL
#define RSF_USE_THREAD_LOCAL 1
#endif

#if RSF_USE_THREAD_LOCAL
#  ifdef _MSC_VER
#    define RSF_THREAD_LOCAL __declspec(thread)
#  else
#    define RSF_THREAD_LOCAL _Thread_local
#  endif
#else
#  define RSF_THREAD_LOCAL
#endif

typedef struct rsf_iface {
    struct rsf_iface* (*trim)(void);
    struct rsf_iface* (*append)(rs_sv);
    struct rsf_iface* (*replace_all)(rs_sv, rs_sv);
    struct rsf_iface* (*printf_)(const char*, ...);
    const char* (*cstr)(void);
    void (*free_)(void);
} rsf_iface;

static RSF_THREAD_LOCAL rs_string* rsf_cur = NULL;
static inline rsf_iface* rsf__ret(void) { static rsf_iface x; (void)x; return &x; }

static inline rsf_iface* rsf_trim(void) { rs_string_trim(rsf_cur); return rsf__ret(); }
static inline rsf_iface* rsf_append(rs_sv v) { rs_string_append(rsf_cur, v); return rsf__ret(); }
static inline rsf_iface* rsf_replace_all(rs_sv a, rs_sv b) { rs_string_replace_all(rsf_cur,a,b); return rsf__ret(); }
static inline rsf_iface* rsf_printf_(const char* fmt, ...) { va_list ap; va_start(ap,fmt); rs_string_vprintf(rsf_cur,fmt,ap); va_end(ap); return rsf__ret(); }
static inline const char* rsf_cstr(void) { return rs_string_cstr(rsf_cur); }
static inline void rsf_free_(void) { rs_string_free(rsf_cur); }

static inline rsf_iface* rsf_table(void) {
    static rsf_iface api = { rsf_trim, rsf_append, rsf_replace_all, rsf_printf_, rsf_cstr, rsf_free_ };
    return &api;
}
static inline rsf_iface* RS(rs_string* s){ rsf_cur = s; return rsf_table(); }

#endif /* RS_ENABLE_FLUENT */
