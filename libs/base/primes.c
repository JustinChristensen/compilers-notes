#include <limits.h>
#include "base/macros.h"
#include "base/primes.h"

unsigned int const primes[] = {
    7,13,19,29,47,71,113,179,283,457,727,1163,1847,2953,4729,7559,
    12097,19373,30971,49523,79231,126781,202841,324523,519247,830777,
    1329233, 2126767,3402851,5444519,8711237,13937999,22300763,35681213,
    57089911,91343891,146150167,233840273,374144431,598631083,
    957809729,1532495599,2451992897,3923188669
};

unsigned int const *const start_prime = primes;
unsigned int const *const end_prime = primes + (SIZEOF(primes) - 1);
