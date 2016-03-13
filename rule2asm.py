# rule2asm.py -- convert an outer-totalistic rule into x86 assembly code using AVX.

import sys
import re

def printinstr(g, s):

    g.write('                "'+s+' \\n\\t"\n')

def varvex3(g, regsize, op, inreg, inreg2, outreg):

    if (regsize == 256):
        printinstr(g, 'vp'+op+' %%ymm'+str(inreg)+', %%ymm'+str(inreg2)+', %%ymm'+str(outreg))
    elif (regsize == 192):
        printinstr(g, 'vp'+op+' %%xmm'+str(inreg)+', %%xmm'+str(inreg2)+', %%xmm'+str(outreg))
    else:
        if (inreg2 == outreg):
            printinstr(g, 'p'+op+' %%xmm'+str(inreg)+', %%xmm'+str(outreg))
        elif (inreg == outreg):
            if (op == 'andn'):
                printinstr(g, 'por %%xmm'+str(inreg2)+', %%xmm'+str(outreg))
                printinstr(g, 'pxor %%xmm'+str(inreg2)+', %%xmm'+str(outreg))
            else:
                printinstr(g, 'p'+op+' %%xmm'+str(inreg2)+', %%xmm'+str(outreg))
        else:
            printinstr(g, 'movdqa %%xmm'+str(inreg2)+', %%xmm'+str(outreg))
            printinstr(g, 'p'+op+' %%xmm'+str(inreg)+', %%xmm'+str(outreg))

def varvex(g, regsize, op, inreg, outreg):

    if (regsize == 256):
        printinstr(g, 'vp'+op+' %%ymm'+str(inreg)+', %%ymm'+str(outreg)+', %%ymm'+str(outreg))
    elif (regsize == 192):
        printinstr(g, 'vp'+op+' %%xmm'+str(inreg)+', %%xmm'+str(outreg)+', %%xmm'+str(outreg))
    else:
        printinstr(g, 'p'+op+' %%xmm'+str(inreg)+', %%xmm'+str(outreg))

def printcomment(g, s):

    g.write('                // '+s+'\n')

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

def genlogic(g, rulestring, regsize):

    bee = [0] * 10
    ess = [0] * 10

    for char in rulestring:
        if (char == 'b'):
            birth = True
        elif (char == 's'):
            birth = False
        else:
            k = int(char)
            if (birth):
                bee[k] = 1
            else:
                ess[k+1] = 1

    # print(bee)
    # print(ess)

    beexor = (bee[0] != bee[8])
    essxor = (ess[1] != ess[9])

    stars = ("*" if essxor else "") + ("**" if beexor else "")

    if (essxor):
        ess[0] = 1 - ess[8]
    else:
        ess[0] = ess[8]

    usetopbit = (essxor or beexor)

    printcomment(g, 'extract bits of neighbour count:')
    varvex3(g, regsize, 'and', 8, 11, 1)
    varvex(g, regsize, 'xor', 11, 8)
    if (beexor and not essxor):
        varvex3(g, regsize, 'and', 1, 9, 0)
        varvex3(g, regsize, 'andn', 0, 12, 11)
    elif (usetopbit):
        varvex3(g, regsize, 'and', 1, 9, 11)
    if (essxor and not beexor):
        varvex(g, regsize, 'and', 12, 11)
    varvex(g, regsize, 'xor', 1, 9)

    ruleint = 0;
    for i in xrange(8):
        ruleint += (bee[i] << i)
        ruleint += (ess[i] << (i + 8))

    print("Rule integer:     "+str(ruleint)+stars)
    regnames = [10, 8, 9, 12, 1, 0]
    opnames = ["and", "or", "andn", "nonsense", "xor"]
    rident = None

    with open('includes/boolean.out', 'r') as f:
        for fline in f:
            x = fline.split(":  ")
            if (len(x) == 2) and (int(x[0]) == ruleint):
                rident = x[1][:-1]

    if (ruleint == 0):
        rident = '-004'
    if (ruleint == 65280):
        rident = '-331'
    if (ruleint == 61680):
        rident = '-221'
    if (ruleint == 52428):
        rident = '-111'
    if (ruleint == 43690):
        rident = '-001'

    if rident is None:
        print("Error: unrecognised rule")
        exit(1)
    else:
        print("Rule circuit:     ["+rident+"]"+stars)

    rchars = list(rident)
    for i in xrange(len(rchars)):
        if (rchars[i] == '-'):
            rchars[i] = (i/4) + 4
        else:
            rchars[i] = int(rchars[i])

    for i in xrange(0, len(rchars), 4):
        if (rchars[i+3] == 3):
            rchars[i+3] = 2
            rchars[i+2] ^= rchars[i+1]
            rchars[i+1] ^= rchars[i+2]
            rchars[i+2] ^= rchars[i+1]

    # print(rchars)

    # Map logical registers to physical registers:
    for i in xrange(0, len(rchars), 4):
        dependencies = [False] * 10
        for j in xrange(i+4, len(rchars), 4):
            dependencies[rchars[j+1]] = True
            dependencies[rchars[j+2]] = True
        d = -1
        e = rchars[i]
        for j in xrange(6):
            if (e == j):
                d = j
                break
            if not dependencies[j]:
                d = j
                break
        if (d == -1):
            print("Error: insufficiently many physical registers")
            exit(1)
        else:
            # print(str(e)+" --> "+str(d))
            rchars[i] = d
            for j in xrange(i+4, len(rchars), 4):
                if (rchars[j+1] == e):
                    rchars[j+1] = d
                if (rchars[j+2] == e):
                    rchars[j+2] = d

    # print(rchars)

    printcomment(g, 'compute appropriate quaternary Boolean function:')
    if (rulestring == 'b3s23'):
        # Possibly more optimal due to instruction order:
        varvex(g, regsize, 'xor', 9, 8)
        varvex(g, regsize, 'or', 10, 12)
        varvex(g, regsize, 'xor', 9, 10)
        varvex(g, regsize, 'and', 12, 8)
        varvex(g, regsize, 'and', 8, 10)
    else:
        for i in xrange(0, len(rchars), 4):
            varvex3(g, regsize, opnames[rchars[i+3]], regnames[rchars[i+1]], regnames[rchars[i+2]], regnames[rchars[i]])

    if (usetopbit):
        printcomment(g, 'correct for B8/S8 nonsense:')
        varvex(g, regsize, 'xor', 11, 10)


def applyrule(g, quadrows, oddgen, regsize, historical=True):

    if historical or not oddgen:
        g.write('        asm (\n')


    # Load addresses into registers:
    if (regsize == 256):
        accessor = 'vmovdqu '
        regbytes = 32
        if (oddgen):
            # printinstr(g, accessor+str(quadrows * 32)+'(%1), %%ymm14')
            printinstr(g, accessor+'(%2), %%ymm14')
        if historical or not oddgen:
            # printinstr(g, accessor+str(quadrows * 32 + 32)+'(%1), %%ymm13')
            printinstr(g, accessor+'32(%2), %%ymm13')
    else:
        regbytes = 16
        accessor = 'vmovdqu ' if (regsize == 192) else 'movups '
        if historical or not oddgen:
            printinstr(g, "mov $0xffffffff, %%ebx")
            printinstr(g, "movd %%ebx, %%xmm13")
            printinstr(g, "mov $0x3ffffffc, %%ebx")
            printinstr(g, "movd %%ebx, %%xmm14")
            if (regsize == 192):
                printinstr(g, "vpshufd $1, %%xmm13, %%xmm13")
                printinstr(g, "vpshufd $0, %%xmm14, %%xmm14")
            else:
                printinstr(g, "pshufd $1, %%xmm13, %%xmm13")
                printinstr(g, "pshufd $0, %%xmm14, %%xmm14")


    for i in xrange(quadrows + (0 if (oddgen and (regbytes == 16)) else 1)):

        if (i < quadrows):
            # Source register:
            d = '(%1)' if (oddgen) else '(%0)'
            d = d if (i == 0) else (str(regbytes * i) + d)

            printcomment(g, 'calculate row-wise parity and carry:')
            if (i % 2 == 0):
                printinstr(g, accessor+d+(', %%ymm5' if (regsize == 256) else ', %%xmm5'))
                if (regsize == 256):
                    printinstr(g, 'vpsrld $1, %%ymm5, %%ymm0')
                    printinstr(g, 'vpslld $1, %%ymm5, %%ymm1')
                elif (regsize == 192):
                    printinstr(g, 'vpsrld $1, %%xmm5, %%xmm0')
                    printinstr(g, 'vpslld $1, %%xmm5, %%xmm1')
                else:
                    printinstr(g, 'movdqa %%xmm5, %%xmm0')
                    printinstr(g, 'movdqa %%xmm5, %%xmm1')
                    printinstr(g, 'psrld $1, %%xmm0')
                    printinstr(g, 'pslld $1, %%xmm1')

                varvex3(g, regsize, 'xor', 0, 1, 6)
                varvex3(g, regsize, 'and', 0, 1, 7)
                varvex3(g, regsize, 'and', 5, 6, 1)
                varvex(g, regsize, 'xor', 5, 6)
                varvex(g, regsize, 'or', 1, 7)
            else:
                printinstr(g, accessor+d+(', %%ymm2' if (regsize == 256) else ', %%xmm2'))
                if (regsize == 256):
                    printinstr(g, 'vpsrld $1, %%ymm2, %%ymm0')
                    printinstr(g, 'vpslld $1, %%ymm2, %%ymm1')
                elif (regsize == 192):
                    printinstr(g, 'vpsrld $1, %%xmm2, %%xmm0')
                    printinstr(g, 'vpslld $1, %%xmm2, %%xmm1')
                else:
                    printinstr(g, 'movdqa %%xmm2, %%xmm0')
                    printinstr(g, 'movdqa %%xmm2, %%xmm1')
                    printinstr(g, 'psrld $1, %%xmm0')
                    printinstr(g, 'pslld $1, %%xmm1')

                varvex3(g, regsize, 'xor', 0, 1, 3)
                varvex3(g, regsize, 'and', 0, 1, 4)
                varvex3(g, regsize, 'and', 2, 3, 1)
                varvex(g, regsize, 'xor', 2, 3)
                varvex(g, regsize, 'or', 1, 4)

        if (i > 0):
            # Destination register:
            e = '(%0)' if (oddgen) else '(%1)'
            if oddgen:
                e = str(regbytes * (i-1) + 8) + e
            else:
                e = e if (i == 1) else (str(regbytes * (i - 1)) + e)

            printcomment(g, 'apply vertical bitshifts:')

            if (regsize == 256):
                blend1 = 'vpblendd $1,'
                blend2 = 'vpblendd $3,'
                if (i % 2 == 0):
                    printinstr(g, blend1+' %%ymm6, %%ymm3, %%ymm8')
                    printinstr(g, blend1+' %%ymm7, %%ymm4, %%ymm9')
                    printinstr(g, blend2+' %%ymm6, %%ymm3, %%ymm10')
                    printinstr(g, blend2+' %%ymm7, %%ymm4, %%ymm11')
                    printinstr(g, blend1+' %%ymm5, %%ymm2, %%ymm12')
                else:
                    printinstr(g, blend1+' %%ymm3, %%ymm6, %%ymm8')
                    printinstr(g, blend1+' %%ymm4, %%ymm7, %%ymm9')
                    printinstr(g, blend2+' %%ymm3, %%ymm6, %%ymm10')
                    printinstr(g, blend2+' %%ymm4, %%ymm7, %%ymm11')
                    printinstr(g, blend1+' %%ymm2, %%ymm5, %%ymm12')
                printinstr(g, 'vpermd %%ymm8, %%ymm13, %%ymm8')
                printinstr(g, 'vpermd %%ymm9, %%ymm13, %%ymm9')
                printinstr(g, 'vpermq $57, %%ymm10, %%ymm10')
                printinstr(g, 'vpermq $57, %%ymm11, %%ymm11')
                printinstr(g, 'vpermd %%ymm12, %%ymm13, %%ymm12')
            elif (regsize == 192):
                printinstr(g, 'vpand %%xmm13, %%xmm'+str(3 + 3*(i%2))+', %%xmm8')
                printinstr(g, 'vpand %%xmm13, %%xmm'+str(4 + 3*(i%2))+', %%xmm9')
                printinstr(g, 'vpand %%xmm13, %%xmm'+str(2 + 3*(i%2))+', %%xmm12')

                printinstr(g, 'vpandn %%xmm'+str(6 - 3*(i%2))+', %%xmm13, %%xmm0')
                printinstr(g, 'vpor %%xmm0, %%xmm8, %%xmm8')

                printinstr(g, 'vpandn %%xmm'+str(7 - 3*(i%2))+', %%xmm13, %%xmm0')
                printinstr(g, 'vpor %%xmm0, %%xmm9, %%xmm9')

                printinstr(g, 'vpandn %%xmm'+str(5 - 3*(i%2))+', %%xmm13, %%xmm0')
                printinstr(g, 'vpor %%xmm0, %%xmm12, %%xmm12')

                printinstr(g, 'vshufps $0x39, %%xmm8, %%xmm8, %%xmm8')
                printinstr(g, 'vshufps $0x39, %%xmm9, %%xmm9, %%xmm9')
                printinstr(g, 'vshufps $0x4e, %%xmm'+str(6 - 3*(i%2))+', %%xmm'+str(3 + 3*(i%2))+', %%xmm10')
                printinstr(g, 'vshufps $0x4e, %%xmm'+str(7 - 3*(i%2))+', %%xmm'+str(4 + 3*(i%2))+', %%xmm11')
                printinstr(g, 'vshufps $0x39, %%xmm12, %%xmm12, %%xmm12')
            else:
                printinstr(g, 'movdqa %%xmm'+str(3 + 3*(i%2))+', %%xmm8')
                printinstr(g, 'movdqa %%xmm'+str(4 + 3*(i%2))+', %%xmm9')
                printinstr(g, 'movdqa %%xmm'+str(3 + 3*(i%2))+', %%xmm10')
                printinstr(g, 'movdqa %%xmm'+str(4 + 3*(i%2))+', %%xmm11')
                printinstr(g, 'movdqa %%xmm'+str(2 + 3*(i%2))+', %%xmm12')

                printinstr(g, 'movdqa %%xmm13, %%xmm0')
                printinstr(g, 'pand %%xmm13, %%xmm8')
                printinstr(g, 'pandn %%xmm'+str(6 - 3*(i%2))+', %%xmm0')
                printinstr(g, 'por %%xmm0, %%xmm8')

                printinstr(g, 'movdqa %%xmm13, %%xmm0')
                printinstr(g, 'pand %%xmm13, %%xmm9')
                printinstr(g, 'pandn %%xmm'+str(7 - 3*(i%2))+', %%xmm0')
                printinstr(g, 'por %%xmm0, %%xmm9')

                printinstr(g, 'movdqa %%xmm13, %%xmm0')
                printinstr(g, 'pand %%xmm13, %%xmm12')
                printinstr(g, 'pandn %%xmm'+str(5 - 3*(i%2))+', %%xmm0')
                printinstr(g, 'por %%xmm0, %%xmm12')

                printinstr(g, 'shufps $0x39, %%xmm8, %%xmm8')
                printinstr(g, 'shufps $0x39, %%xmm9, %%xmm9')
                printinstr(g, 'shufps $0x4e, %%xmm'+str(6 - 3*(i%2))+', %%xmm10')
                printinstr(g, 'shufps $0x4e, %%xmm'+str(7 - 3*(i%2))+', %%xmm11')
                printinstr(g, 'shufps $0x39, %%xmm12, %%xmm12')

            printcomment(g, 'apply vertical full-adders:')
            varvex(g, regsize, 'xor', 3 + 3*(i%2), 8)
            varvex(g, regsize, 'xor', 4 + 3*(i%2), 9)
            varvex(g, regsize, 'xor', 8, 10)
            varvex(g, regsize, 'xor', 9, 11)
            varvex(g, regsize, 'or', 8, 3 + 3*(i%2))
            varvex(g, regsize, 'or', 9, 4 + 3*(i%2))
            varvex(g, regsize, 'and', 10, 8)
            varvex(g, regsize, 'and', 11, 9)
            varvex(g, regsize, 'andn', 3 + 3*(i%2), 8)
            varvex(g, regsize, 'andn', 4 + 3*(i%2), 9)

            g.write('#include "lifelogic.h"\n')

            if (oddgen):
                printcomment(g, 'determine diff:')
                if (i == quadrows):
                    # Can only happen in 256-bit mode:
                    # printcomment(g, 'dodgy vertical bit-masking hack:')
                    # printinstr(g, 'vpxor %%ymm14, %%ymm14, %%ymm0')
                    # printinstr(g, 'vpblendd $15, %%ymm14, %%ymm0, %%ymm14')
                    varvex(g, 192, 'and', 14, 10)
                    printinstr(g, accessor + e + ', %%xmm8')
                    varvex3(g, 192, 'andn', 8, 14, 11)
                    varvex(g, 192, 'or', 10, 11)
                    printinstr(g, accessor + '%%xmm11, ' + e)

                else:
                    varvex(g, regsize, 'and', 14, 10)
                    printinstr(g, accessor + e + (', %%ymm8' if (regsize == 256) else ', %%xmm8'))
                    varvex3(g, regsize, 'andn', 8, 14, 11)
                    varvex(g, regsize, 'or', 10, 11)
                    printinstr(g, accessor + ('%%ymm11, ' if (regsize == 256) else '%%xmm11, ') + e)

                if (i == 1):
                    varvex3(g, regsize, 'xor', 11, 8, 15)
                    printcomment(g, 'save diff:')
                    printinstr(g, accessor + ('%%ymm15, ' if (regsize == 256) else '%%xmm15, ') + '(%1)')
                else:
                    varvex(g, regsize, 'xor', 11, 8)
                    varvex(g, regsize, 'or', 8, 15)

                if (i == (quadrows - (1 if (regbytes == 16) else 0))):
                    printcomment(g, 'save diffs:')
                    if (regsize == 256):
                        printinstr(g, accessor + '%%ymm8, 64(%1)')
                        printinstr(g, accessor + '%%ymm15, 32(%1)')
                    else:
                        printinstr(g, accessor + '%%xmm8, 32(%1)')
                        printinstr(g, accessor + '%%xmm15, 16(%1)')
            else:
                printcomment(g, 'store result:')
                printinstr(g, accessor + ('%%ymm10, ' if (regsize == 256) else '%%xmm10, ') + e)

    if oddgen or historical:
        g.write('                : /* no output operands */ \n')
        if (regsize == 256):
            g.write('                : "r" (sqt->d), "r" (e), "r" (globarray) \n')
        else:
            g.write('                : "r" (sqt->d), "r" (e) \n')
        g.write('                : ')
        if (regsize != 256):
            g.write('"ebx", ')
        for i in xrange(16):
            g.write('"xmm'+str(i)+'", ')
        g.write('"memory");\n\n')


def genasm(g, quadrows, rulestring, regsize):

    g.write('\n\n        // Code generated by rule2asm.py\n\n')
    g.write('        // Tile size: 32 * '+str((regsize / 32) * quadrows)+'\n')
    g.write('        // Rule: '+rulestring+'\n\n')

    g.write('        uint32_t e[ROWS];\n\n')

    # g.write('        if (history) {for (int i = 2; i < ROWS - 2; i++) {sqt->hist[i] |= sqt->d[i]; }}\n')
    # applyrule(g, quadrows, False, regsize)
    # g.write('        if (history) {for (int i = 2; i < ROWS - 2; i++) {sqt->hist[i] |= e[i-1]; }}\n')
    # applyrule(g, quadrows, True, regsize)

    g.write('        if (history) {\n')
    applyrule(g, quadrows, False, regsize)
    g.write('        for (int i = 2; i < ROWS - 2; i++) {sqt->hist[i] |= sqt->d[i]; }\n')
    applyrule(g, quadrows, True, regsize)
    g.write('        for (int i = 2; i < ROWS - 2; i++) {sqt->hist[i] |= e[i-1]; }\n')
    g.write('        } else {\n')
    applyrule(g, quadrows, False, regsize, historical=False)
    applyrule(g, quadrows, True, regsize, historical=False)
    g.write('        }\n')

    g.write('        // The diffs we\'re interested in:\n')
    if (regsize == 256):
        g.write('        uint32_t topdiff = e[0] | e[1];\n')
        g.write('        uint32_t bigdiff = e[8] | e[9] | e[10] | e[11] | e[12] | e[13] | e[14] | e[15];\n')
        g.write('        uint32_t botdiff = e[18] | e[19];\n')
    else:
        g.write('        uint32_t topdiff = e[0] | e[1];\n')
        g.write('        uint32_t bigdiff = e[4] | e[5] | e[6] | e[7];\n')
        g.write('        uint32_t botdiff = e[10] | e[11];\n')

    g.write('\n\n        // End of generated code\n\n')


def main():

    if (len(sys.argv) < 3):
        print("Usage:")
        print("python rule2asm.py b3s23 avx2")
        print("python rule2asm.py b3678s34678 sse2")

    rulestring = sys.argv[1]
    machinetype = sys.argv[2]

    m = re.match('b3?4?5?6?7?8?s0?1?2?3?4?5?6?7?8?$', rulestring)
    if m is None:
        print("Invalid rulestring: \033[1;31m"+rulestring+"\033[0m is not of the form:")
        print("    bXsY\nwhere X is a subset of {3, ..., 8} and Y is a subset of {0, ..., 8}")
        exit(1)
    else:
        print("Valid rulestring: \033[1;32m"+m.group(0)+"\033[0m")

    (bee, ess) = binary_rulestring(rulestring)

    if (machinetype == 'avx2'):
        quadrows = 4
        regsize = 256
        simdrows = 8
    elif (machinetype == 'avx1'):
        quadrows = 8
        regsize = 192
        simdrows = 4
    elif (machinetype == 'sse2'):
        quadrows = 8
        regsize = 128
        simdrows = 4
    else:
        print("Machine type must be either 'avx2', 'avx1', or 'sse2'.")
        exit(1)

    with open('includes/lifelogic.h', 'w') as g:

        genlogic(g, rulestring, regsize)

    with open('includes/lifeasm.h', 'w') as g:

        genasm(g, quadrows, rulestring, regsize)

    with open('includes/params.h', 'w') as g:

        g.write('#define RULESTRING "'+rulestring+'"\n')
        g.write('#define RULESTRING_SLASHED "'+rulestring.replace('b', 'B').replace('s', '/S')+'"\n')
        g.write('#define QUADROWS '+str(quadrows)+'\n')
        g.write('#define BITTAGE 32\n')
        g.write('#define ROWS '+str(quadrows * simdrows)+'\n')
        g.write('#define THSPACE 28\n')
        g.write('#define MIDDLE28 0x3ffffffcu\n')
        g.write('#define BIRTHS '+str(bee)+'\n')
        g.write('#define SURVIVALS '+str(ess)+'\n')
        g.write('typedef uint32_t urow_t;\n')
        if (rulestring == 'b3s23'):
            g.write('#define STANDARD_LIFE 1\n')


main()

print("Success!")
