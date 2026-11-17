#include <stdio.h>
#include "collatz.h"

int
main(void)
{
    long long lower = 1, upper = 1000000;
    long long longest = 0;
    int length = 0;

    for (long long n = lower; n < upper; n++) {
        int l = collatz(n);
        if (l > length) {
            longest = n;
            length = l;
        }
    }
    printf("longest collatz sequence in [%lld:%lld) is at %lld (length %d)\n",
           lower, upper, longest, length);
    return 0;
}
