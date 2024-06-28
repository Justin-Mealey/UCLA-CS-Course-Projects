#ifndef RAND64HW_H
#define RAND64HW_H

#include <stdbool.h>
#include <cpuid.h>
#include <immintrin.h>

struct cpuid { unsigned eax, ebx, ecx, edx; };

struct cpuid
cpuid (unsigned int leaf, unsigned int subleaf);

bool
rdrand_supported (void);

void
hardware_rand64_init (void);

unsigned long long
hardware_rand64 (void);

void
hardware_rand64_fini (void);

#endif