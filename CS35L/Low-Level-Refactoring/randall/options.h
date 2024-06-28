#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>

bool argparse(int argc, char **argv);

char* getInput();
char* getOutput();
char* getInputArg();
char* getOutputArg();

#endif