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

#include <stdio.h>
#include "rs_string.h"
#include "rs_string_api.h"

int main(void) {
    rs_string s = rs_string_from_val("  Hello");
    rs()->trim(&s);
    rs()->append(&s, rs_sv_from_cstr(", rs"));
    rs()->replace_all(&s, rs_sv_from_cstr("rs"), rs_sv_from_cstr("rs_string"));
    rs()->printf_(&s, "[%s] len=%zu", rs()->cstr(&s), rs()->len(&s));
    puts(rs()->cstr(&s));
    rs()->free_(&s);
    return 0;
}
