#include "collatz.h"

int
collatz(long long n)
{
    int i = 1;

    while (n != 1) {
        if (n % 2 == 0) {
            n = n / 2;
        } else {
            n = 3 * n + 1;
        }
        i++;
    }
    return i;
}
