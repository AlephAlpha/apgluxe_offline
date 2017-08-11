#!/usr/bin/python

import sys
import re

def binary_rulestring(rulestring):

    bee = 0
    ess = 0

    for char in rulestring:
        if (char == 'b'):
            birth = True
        elif (char == 's'):
            birth = False
        else:
            k = int(char)
            if (birth):
                bee = bee | (1 << k)
            else:
                ess = ess | (1 << k)

    return (bee, ess)

def main():

    if (len(sys.argv) < 3):
        print("Usage:")
        print("python mkparams.py b3s23 C1")
        exit(1)

    rulestring = sys.argv[1]
    symmetry = sys.argv[2]

    validsyms = ["C1", "C2_4", "C2_2", "C2_1", "C4_4", "C4_1", "8x32", "4x64", "2x128", "1x256",
                 "D2_+2", "D2_+1", "D2_x", "D4_+4", "D4_+2", "D4_+1", "D4_x4", "D4_x1", "D8_4", "D8_1"]

    if symmetry not in validsyms:
        print("Invalid symmetry: \033[1;31m"+symmetry+"\033[0m is not one of the supported symmetries:")
        print(repr(validsyms))
        exit(1)

    print("Valid symmetry: \033[1;32m"+symmetry+"\033[0m")

    (bee, ess) = binary_rulestring(rulestring)

    with open('includes/params.h', 'w') as g:

        g.write('#define SYMMETRY "'+symmetry+'"\n')
        g.write('#define RULESTRING "'+rulestring+'"\n')
        g.write('#define RULESTRING_SLASHED "'+rulestring.replace('b', 'B').replace('s', '/S')+'"\n')
        g.write('#define BIRTHS '+str(bee)+'\n')
        g.write('#define SURVIVALS '+str(ess)+'\n')
        if (symmetry == 'C1'):
            g.write('#define C1_SYMMETRY 1\n')
        if (rulestring == 'b3s23'):
            g.write('#define STANDARD_LIFE 1\n')
        if (re.match('b36?7?8?s0?235?6?7?8?$', rulestring)):
            g.write('#define GLIDERS_EXIST 1\n')


main()

print("Success!")
