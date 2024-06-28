#include "options.h"

//based on options given
char *input;
char *output;

//which options we use to calculate/output random bytes
//input options
bool rdrand_set = true;
bool lrand48_set = false;
bool file_set = false;
//output options
bool stdio_set = true;


bool argparse(int argc, char **argv){ //set args, how to calculate, how to output
    int opt;
    while ((opt = getopt(argc, argv, ":i:o:")) != -1) {
        switch(opt) {
            case 'i':
                if (argc != 4 && argc != 6){
                    fprintf (stderr, "%s: usage: %s NBYTES\n", argv[0], argv[0]);
                    return false;
                }
                if (optarg[0]=='/') {
                    file_set = true;
                    rdrand_set = false;
                }
                else if (strcmp("lrand48_r", optarg) == 0) {
                    rdrand_set = false;
                    lrand48_set = true;
                }
                else if (strcmp("rdrand", optarg) != 0) { //none of the specified -i options entered
                    fprintf(stderr, "Invalid input option for -i\n");
                    return false;
                }
                input = optarg;
                break;
            case 'o':
                if (isdigit(*optarg)){
                    stdio_set = false;
                }
                else if (strcmp("stdio", optarg) != 0){ //none of the specified -o options entered
                    fprintf(stderr,"Inavlid output option for -o\n", optopt);
                    return false;
                }
                output = optarg;
                break;
            case ':': 
                fprintf(stderr,"Every option must have an operand\n");
                return false;
            case '?':
                fprintf(stderr,"Only options are -i and -o\n");
                return false;
        }
    }
    return true;
}


char* getInput(){
    if(rdrand_set){
        return "rdrand";
    }
    else if(lrand48_set){
        return "lrand48";
    }
    else if(file_set){
        return "file";
    }
    else{ //should never be hit, but just in case
        fprintf(stderr,"No correct input given\n");
        return "N/A";
    }
}

char* getOutput(){
    if(stdio_set){
        return "stdio";
    }
    else {
        return "N";
    }
}

char* getInputArg(){
    return input;
}
char* getOutputArg(){
    return output;
}