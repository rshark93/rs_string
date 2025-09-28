
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "rs_string.h"

static void test_basic() {
    rs_string s = rs_string_from_val("Hello");
    assert(rs_string_len(&s) == 5);
    assert(rs_string_cap(&s) > 5);

    rs_string_append(&s, rs_sv_from_cstr(", world"));
    assert(strcmp(rs_string_cstr(&s), "Hello, world") == 0);

    size_t pos = rs_string_find(&s, rs_sv_from_cstr("world"), 0);
    assert(pos == 7);

    rs_string_erase(&s, pos, 5);
    rs_string_insert(&s, pos, rs_sv_from_cstr("rs_string"));
    assert(strcmp(rs_string_cstr(&s), "Hello, rs_string") == 0);

    rs_string_free(&s);
}

static void test_cow() {
    rs_string a = rs_string_from_val("data");
    rs_string b;
    rs_string_init(&b);
    rs_string_share(&b, &a); // b shares "data"
    rs_string_append(&a, rs_sv_from_cstr("X")); // COW triggers
    assert(strcmp(rs_string_cstr(&a), "dataX") == 0);
    assert(strcmp(rs_string_cstr(&b), "data") == 0);
    rs_string_free(&a);
    rs_string_free(&b);
}


static void test_trim_split_replace() {
    rs_string s = rs_string_from_val(" \t hi  ");
    rs_string_trim(&s);
    assert(strcmp(rs_string_cstr(&s), "hi") == 0);

    int tokens = 0;
    rs_sv_split(rs_sv_from_cstr("a,,b,"), rs_sv_from_cstr(","), 1, cb_count, &tokens);
    assert(tokens == 4); /* a, "", b, "" */

    rs_string r = rs_string_from_val("one fish two fish");
    rs_string_replace_all(&r, rs_sv_from_cstr("fish"), rs_sv_from_cstr("cat"));
    assert(strcmp(rs_string_cstr(&r), "one cat two cat") == 0);
    rs_string_replace_first(&r, rs_sv_from_cstr("cat"), rs_sv_from_cstr("dog"));
    assert(strcmp(rs_string_cstr(&r), "one dog two cat") == 0);

    rs_string_free(&s);
    rs_string_free(&r);
}

static void test_utf_converters() {
    /* UTF-16 <-> UTF-8 */
    unsigned char* u16 = NULL;
    size_t u16len = 0;

    assert(0 == rs_utf16_from_utf8_bytes(rs_sv_from_cstr("Hi 🐍"), 1, 1, (rs_alloc){0}, &u16, &u16len));
    rs_string u8; rs_string_init(&u8);
    assert(0 == rs_utf8_from_utf16_bytes(&u8, (rs_sv){(const char*)u16,u16len}, 1));
    assert(strcmp(rs_string_cstr(&u8), "Hi 🐍") == 0);
    free(u16);
    rs_string_free(&u8);

    /* UTF-32 <-> UTF-8 */
    unsigned char* u32 = NULL;
    size_t u32len = 0;
    assert(0 == rs_utf32_from_utf8_bytes(rs_sv_from_cstr("Hi 🐍"), 1, 1, (rs_alloc){0}, &u32, &u32len));
    rs_string u8b; rs_string_init(&u8b);
    assert(0 == rs_utf8_from_utf32_bytes(&u8b, (rs_sv){(const char*)u32,u32len}, 1));
    assert(strcmp(rs_string_cstr(&u8b), "Hi 🐍") == 0);
    free(u32);
    rs_string_free(&u8b);

    /* UTF-16 <-> ASCII */
    unsigned char* u16a = NULL; size_t u16alen = 0;
    assert(0 == rs_utf16_from_ascii_bytes(rs_sv_from_cstr("Hello"), 1, 1, (rs_alloc){0}, &u16a, &u16alen));
    rs_string asc; rs_string_init(&asc);
    assert(0 == rs_ascii_from_utf16_bytes(&asc, (rs_sv){(const char*)u16a,u16alen}, 1, '?'));
    assert(strcmp(rs_string_cstr(&asc), "Hello") == 0);
    free(u16a);
    rs_string_free(&asc);
}

int main(void) {
    test_basic();
    test_cow();
    test_trim_split_replace();
    test_utf_converters();
    puts("All tests passed.");
    return 0;
}
