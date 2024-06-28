#ifndef RAND64SW_H
#define RAND64SW_H

#include <stdio.h>
#include <stdlib.h>

void
software_rand64_init (void);

unsigned long long
software_rand64 (void);

void
reassignFile(char *file);

void
software_rand64_fini (void);

void
reassignFile(char *file);

#endif