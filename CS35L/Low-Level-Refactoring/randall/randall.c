/* Generate N bytes of random output.  */

/* When generating output this program uses the x86-64 RDRAND
   instruction if available to generate random numbers, falling back
   on /dev/random and stdio otherwise.

   This program is not portable.  Compile it with gcc -mrdrnd for a
   x86-64 machine.

   Copyright 2015, 2017, 2020 Paul Eggert

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "options.h"
#include "output.h"
#include "rand64-hw.h"
#include "rand64-sw.h"
#include "lrand48.h"

#include <errno.h>

/* Main program, which outputs N bytes of random data.  */
int
main (int argc, char **argv)
{
  bool valid_arguments = argparse(argc, argv);
  if (valid_arguments == false){
    exit(1);
  }


  long long nbytes = atol(argv[optind]);
  /* If there's no work to do, don't worry about which library to use.  */
  if (nbytes == 0)
    return 0;
  
  void (*initialize) (void);
  unsigned long long (*rand64) (void);
  void (*finalize) (void);

  char* input = getInput();
  if (strcmp(input, "lrand48") == 0){
    initialize = lrand48_r_init_caller;
    rand64 = lrand48_r_caller;
    finalize = lrand48_r_fini_caller;
  }
  else if (strcmp(input, "rdrand") == 0){
    initialize = hardware_rand64_init;
    rand64 = hardware_rand64;
    finalize = hardware_rand64_fini;
  }
  else if (strcmp(input, "file")){ //software computation using file as random data
    initialize = software_rand64_init;
    reassignFile(getInputArg);
    rand64 = software_rand64;
    finalize = software_rand64_fini;  
  }
  else{
    initialize = software_rand64_init;
    rand64 = software_rand64;
    finalize = software_rand64_fini;  
  }
  
  //use values we've just determined
  initialize ();
  int wordsize = sizeof rand64 ();
  int output_errno = 0;
  unsigned long long num = rand64 ();

  char* output = getOutput();
  if (strcmp(output, "N") == 0){ //N at a time algorithm
    char* outputArg = getOutputArg();
    int len = atoi(outputArg);
    if(len == 0){
	    return 0;
    }
    char* buffer = malloc(len);
    do{
        int outbytes = nbytes < len ? nbytes : len;
	      for(int i = 0; i < outbytes; i++){
	        num = rand64();
	        buffer[i] = num;
        }
	  write(1, buffer, outbytes);
	  nbytes -= outbytes;
	  } while(0 < nbytes);
    free(buffer);
  }
  else if (strcmp(output, "stdio") == 0){ //normal output steps
    do{
      unsigned long long x = rand64 ();
      int outbytes = nbytes < wordsize ? nbytes : wordsize;
      if (!writebytes (x, outbytes)){
        output_errno = errno;
	      break;
      }
      nbytes -= outbytes;
    } while (0 < nbytes);
  }
  //cleanup
  if (fclose (stdout) != 0)
    output_errno = errno;

  if (output_errno)
    {
      errno = output_errno;
      perror ("output");
    }

  finalize ();
  return !!output_errno;
}