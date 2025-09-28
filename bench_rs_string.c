
#include <stdio.h>
#include <time.h>
#include "rs_string.h"

static double secs(clock_t a, clock_t b) { return (double)(b - a) / CLOCKS_PER_SEC; }

int main(void) {
    const int N = 200000;
    rs_string s = rs_string_from_val("");
    clock_t t0 = clock();
    for (int i = 0; i < N ; ++i) {
        rs_string_push_cstr(&s, "abcdef");
    }
    clock_t t1 = clock();
    printf("append: %d * 6 bytes -> len=%zu in %.3f s\n", N, rs_string_len(&s), secs(t0,t1));

    clock_t t2 = clock();
    rs_string_replace_all(&s, rs_sv_from_cstr("ab"), rs_sv_from_cstr("AB"));
    clock_t t3 = clock();
    printf("replace_all 'ab'->'AB' in %.3f s\n", secs(t2,t3));

    rs_string_free(&s);
    return 0;
}
