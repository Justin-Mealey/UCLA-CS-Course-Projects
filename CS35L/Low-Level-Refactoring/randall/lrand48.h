#ifndef LRAND48_H
#define LRAND48_H

#include <cpuid.h>
#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void
lrand48_r_init_caller (void);

unsigned long long
lrand48_r_caller (void);

void
lrand48_r_fini_caller (void);

#endif