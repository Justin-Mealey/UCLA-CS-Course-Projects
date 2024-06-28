#!/usr/bin/env python3

"""
Output lines selected randomly from input
"""
import random, sys, argparse

class randline:
    def __init__(self, myinput):
        self.lines = myinput
        random.shuffle(self.lines)

    def chooseline(self, repeatcheck):
        if repeatcheck == True:
            line = random.choice(self.lines)
        else:
            line = self.lines.pop()
        return line

def check_input_range(ir):
    splitarray = ir.split('-')
    
    if len(splitarray) == 2 and splitarray[0].isdigit() and splitarray[1].isdigit() and (int(splitarray[0]) <= int(splitarray[1])):
        return True
    else:
        return False
    
def split_input_range(ir):
    splitarray = ir.split('-')
    return [int(splitarray[0]), int(splitarray[1])]
    
def main():
    version_msg = "%prog 1.0"
    usage_msg = """%prog [-h] [-e [ECHO ...]] [-i INPUT_RANGE] [-r] [-n HEAD_COUNT] [filename ...]

Output randomly selected lines from FILE."""
    
    #GET AND CHECK ARGUMENTS
    
    parser = argparse.ArgumentParser()

    parser.add_argument('-e', '--echo', nargs='*')
    parser.add_argument('-i', '--input-range')
    parser.add_argument('-r', '--repeat', action='store_true')
    parser.add_argument('-n', '--head-count', type=int)
    parser.add_argument('filename', nargs='*')
    
    args = parser.parse_args(sys.argv[1:])
    
    if vars(args)['input_range']!=None:
        ir = vars(args)['input_range']
        valid = check_input_range(ir)
        if not valid:
            strir = str(ir)
            print(f'shuf: invalid input range: \'{strir}\'')
            exit(1)

    if vars(args)['head_count']!=None:
        n = vars(args)['head_count']
        if n < 0:
            strn = str(n)
            print(f'shuf: invalid line count: \'{strn}\'')
            exit(1)
    
    if vars(args)['filename']!=[]:

        if vars(args)['filename']==['-']:
            vars(args)['filename'] = []

        if vars(args)['echo']==None:
            if len(vars(args)['filename']) > 1:
                print('shuf: too many files as input')
                exit(1)
   
    #GET AND CHECK ARGUMENTS 

    #HANDLE INPUT COMBINATIONS

    e = False

    myList = list()
    
    #BASIC CASES
    if vars(args)['echo'] != None:
        e = True
        myList = (vars(args)['echo']) + vars(args)['filename'] 
    elif vars(args)['filename'] != []:
        if vars(args)['input_range'] != None:
            fname = str(vars(args)['filename'][0])
            print(f'shuf: extra operand \'{fname}\'')
            exit(1)
        f = open((vars(args)['filename'])[0], 'r')
        lines = f.readlines()
        for i, line in enumerate(lines):
            lines[i] = line.replace('\n', '')
        myList = lines
        f.close()    
    if vars(args)['input_range'] != None:
        if e:
            print('shuf: cannot combine -e and -i options')
            exit(1)    
        range_array = split_input_range(vars(args)['input_range'])
        myList = (list(range(range_array[0], range_array[1]+1)))
    #BASIC CASES

    
    #GET FROM STDIN
    if (vars(args)['filename'] == []) and (vars(args)['input_range'] == None) and (vars(args)['echo'] == None):
        myList = sys.stdin.readlines()
        for i, line in enumerate(myList):
            myList[i] = line.replace('\n', '')

    myList = randline(myList)
        
    #HANDLE INPUT COMBINATIONS

    n = vars(args)['head_count']
    while True:
        if vars(args)['head_count'] != None:
            if n == 0:
                break
            n -= 1
        if len(myList.lines) == 0:
            return 0
        print(myList.chooseline(vars(args)['repeat']))    
        
        #break
        #else
        #print random line, if ! -r delete
            
            
'''
    try:
        numlines = int(options.numlines)
    except:
        parser.error("invalid NUMLINES: {0}".
                     format(options.numlines))
    if numlines < 0:
        parser.error("negative count: {0}".
                     format(numlines))
    if len(args) != 1:
        parser.error("wrong number of operands")
    input_file = args[0]

    try:
        generator = randline(input_file)
        for index in range(numlines):
            sys.stdout.write(generator.chooseline())
    except IOError as (errno, strerror):
        parser.error("I/O error({0}): {1}".
                     format(errno, strerror))
'''
if __name__ == "__main__":
    main()

