#include "lrand48.h"

struct drand48_data buffer;

void 
lrand48_r_init_caller (void) {
  srand48_r(time(NULL), &buffer);
}

long int first_32;
long int last_32;
unsigned long long 
lrand48_r_caller (void){
  mrand48_r(&buffer, &first_32); //allowed to use mrand instead, much simpler to implement than 31 bit at a time lrand
  mrand48_r(&buffer, &last_32);
  unsigned long long int res = ((unsigned long long int) first_32 | ((unsigned long long int) last_32 << 32));
  return res;
}

void
lrand48_r_fini_caller (void){}