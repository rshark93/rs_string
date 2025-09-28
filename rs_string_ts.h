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
#include <threads.h>
#include <stdarg.h>
#include "rs_string.h"

typedef struct { rs_string s; mtx_t m; } rs_string_ts;
static inline int rs_string_ts_init(rs_string_ts* t) { rs_string_init(&t->s); return mtx_init(&t->m, mtx_plain)==thrd_success?0:-1; }
static inline void rs_string_ts_free(rs_string_ts* t) { mtx_lock(&t->m); rs_string_free(&t->s); mtx_unlock(&t->m); mtx_destroy(&t->m); }
static inline int rs_string_ts_append(rs_string_ts* t, rs_sv v) { mtx_lock(&t->m); int r=rs_string_append(&t->s,v); mtx_unlock(&t->m); return r; }
static inline size_t rs_string_ts_len(rs_string_ts* t) { mtx_lock(&t->m); size_t r=rs_string_len(&t->s); mtx_unlock(&t->m); return r; }
