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


// rs_string_api.h — interface-style API for rs_string (header-only)
// Usage: rs()->append(&s, ...); rs()->trim(&s);
#pragma once
#include "rs_string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rs_api {
    /* lifecycle / info */
    void        (*free_)(rs_string*);
    const char* (*cstr)(const rs_string*);
    size_t      (*len)(const rs_string*);
    size_t      (*cap)(const rs_string*);
    size_t      (*avail)(const rs_string*);
    bool        (*is_heap)(const rs_string*);
    bool        (*is_shared)(const rs_string*);

    /* edit */
    int (*clear)(rs_string*);
    int (*assign)(rs_string*, rs_sv);
    int (*from_cstr)(rs_string*, const char*);
    int (*append)(rs_string*, rs_sv);
    int (*push_cstr)(rs_string*, const char*);
    int (*push_char)(rs_string*, char);
    int (*insert)(rs_string*, size_t pos, rs_sv);
    int (*erase)(rs_string*, size_t pos, size_t n);

    /* find */
    size_t (*find)(const rs_string*, rs_sv what, size_t from);
    int    (*starts_with)(const rs_string*, rs_sv);
    int    (*ends_with)(const rs_string*, rs_sv);

    /* printf */
    int (*printf_)(rs_string*, const char* fmt, ...);

    /* replace */
    int (*replace_first)(rs_string*, rs_sv from, rs_sv to);
    int (*replace_all)(rs_string*,  rs_sv from, rs_sv to);

    /* trim */
    int (*trim)(rs_string*);
    int (*trim_left)(rs_string*);
    int (*trim_right)(rs_string*);
    int (*trim_cut)(rs_string*, rs_sv cut);

    /* share/cow */
    void (*share)(rs_string* dst, const rs_string* src);
    int  (*retain)(rs_string*);
    void (*release)(rs_string*);

    /* unicode helpers (selected) */
    int  (*utf8_from_utf16_bytes)(rs_string* out, rs_sv utf16_bytes, int default_little_endian);
    int  (*utf16_from_utf8_bytes)(rs_sv utf8, int little_endian, int write_bom, rs_alloc a, unsigned char** out_bytes, size_t* out_len_bytes);
    int  (*utf8_from_utf32_bytes)(rs_string* out, rs_sv utf32_bytes, int default_little_endian);
    int  (*utf32_from_utf8_bytes)(rs_sv utf8, int little_endian, int write_bom, rs_alloc a, unsigned char** out_bytes, size_t* out_len_bytes);
} rs_api;

/* singleton accessor */
static inline const rs_api* rs(void){
    /* forward variadic */
    int rs_string_printf(rs_string*, const char*, ...);

    static const rs_api api = {
            /* lifecycle/info */
            .free_       = rs_string_free,
            .cstr        = rs_string_cstr,
            .len         = rs_string_len,
            .cap         = rs_string_cap,
            .avail       = rs_string_avail,
            .is_heap     = rs_string_is_heap,
//            .is_shared   = rs_string_is_shared,

            /* edit */
            .clear       = rs_string_clear,
            .assign      = rs_string_assign,
            .from_cstr   = rs_string_from_cstr,
            .append      = rs_string_append,
            .push_cstr   = rs_string_push_cstr,
            .push_char   = rs_string_push_char,
            .insert      = rs_string_insert,
            .erase       = rs_string_erase,

            /* find */
            .find        = rs_string_find,
            .starts_with = rs_string_starts_with,
            .ends_with   = rs_string_ends_with,

            /* printf */
            .printf_     = rs_string_printf,

            /* replace */
            .replace_first = rs_string_replace_first,
            .replace_all   = rs_string_replace_all,

            /* trim */
            .trim       = rs_string_trim,
            .trim_left  = rs_string_trim_left,
            .trim_right = rs_string_trim_right,
//            .trim_cut   = rs_string_trim_cut,

            /* share/cow */
            .share      = rs_string_share,
//            .retain     = rs_string_retain,
//            .release    = rs_string_release,

            /* unicode helpers */
            .utf8_from_utf16_bytes = rs_utf8_from_utf16_bytes,
            .utf16_from_utf8_bytes = rs_utf16_from_utf8_bytes,
            .utf8_from_utf32_bytes = rs_utf8_from_utf32_bytes,
            .utf32_from_utf8_bytes = rs_utf32_from_utf8_bytes,
    };
    return &api;
}

#ifdef __cplusplus
}
#endif
