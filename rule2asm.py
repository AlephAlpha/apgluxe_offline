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


def applyrule(g, quadrows, oddgen, regsize, historical, machinetype):

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

            g.write('#include "lifelogic-'+machinetype+'.h"\n')

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


def genasm(g, quadrows, rulestring, regsize, historical, machinetype):

    g.write('\n\n    // Code generated by rule2asm.py\n\n')
    g.write('    // Tile size: 32 * '+str((regsize / 32) * quadrows)+'\n')
    g.write('    // Rule: '+rulestring+'\n\n')

    g.write('    void updateTile_'+machinetype+('_history' if historical else '_nohistory')+'(VersaTile* sqt) {\n\n')

    g.write('        uint32_t e[ROWS];\n\n')

    # g.write('        if (history) {for (int i = 2; i < ROWS - 2; i++) {sqt->hist[i] |= sqt->d[i]; }}\n')
    # applyrule(g, quadrows, False, regsize)
    # g.write('        if (history) {for (int i = 2; i < ROWS - 2; i++) {sqt->hist[i] |= e[i-1]; }}\n')
    # applyrule(g, quadrows, True, regsize)

    if (historical):
        g.write('        for (int i = 2; i < ROWS - 2; i++) {sqt->hist[i] |= sqt->d[i]; }\n')
    applyrule(g, quadrows, False, regsize, historical, machinetype)
    if (historical):
        g.write('        for (int i = 2; i < ROWS - 2; i++) {sqt->hist[i] |= e[i-1]; }\n')
    applyrule(g, quadrows, True, regsize, historical, machinetype)

    g.write('        // The diffs we\'re interested in:\n')
    if (regsize == 256):
        g.write('        uint32_t topdiff = e[0] | e[1];\n')
        g.write('        uint32_t bigdiff = e[8] | e[9] | e[10] | e[11] | e[12] | e[13] | e[14] | e[15];\n')
        g.write('        uint32_t botdiff = e[18] | e[19];\n')
    else:
        g.write('        uint32_t topdiff = e[0] | e[1];\n')
        g.write('        uint32_t bigdiff = e[4] | e[5] | e[6] | e[7];\n')
        g.write('        uint32_t botdiff = e[10] | e[11];\n')

    g.write('        if (bigdiff) {\n')
    g.write('            sqt->populationCurrent = false;\n')
    g.write('            sqt->hashCurrent = false;\n')
    g.write('            if (sqt->updateflags == 0) { modified.push_back(sqt); }\n')
    g.write('            sqt->updateflags |= 64;\n')
    g.write('            if (bigdiff & 0x30000000u) { updateNeighbour(sqt, 5); }\n')
    g.write('            if (bigdiff & 0x0000000cu) { updateNeighbour(sqt, 2); }\n')
    g.write('            if (topdiff & 0x3fffc000u) { updateNeighbour(sqt, 0); }\n')
    g.write('            if (topdiff & 0x0003fffcu) { updateNeighbour(sqt, 1); }\n')
    g.write('            if (botdiff & 0x3fffc000u) { updateNeighbour(sqt, 4); }\n')
    g.write('            if (botdiff & 0x0003fffcu) { updateNeighbour(sqt, 3); }\n')
    g.write('        }\n\n')
    g.write('    }\n\n')

def main():

    if (len(sys.argv) < 3):
        print("Usage:")
        print("python rule2asm.py b3s23 C1")
        exit(1)

    rulestring = sys.argv[1]
    symmetry = sys.argv[2]

    m = re.match('b3?4?5?6?7?8?s0?1?2?3?4?5?6?7?8?$', rulestring)
    if m is None:
        print("Invalid rulestring: \033[1;31m"+rulestring+"\033[0m is not of the form:")
        print("    bXsY\nwhere X is a subset of {3, ..., 8} and Y is a subset of {0, ..., 8}")
        exit(1)
    else:
        print("Valid rulestring: \033[1;32m"+m.group(0)+"\033[0m")

    if (symmetry == "C1"):
        cellrows = 32
    elif (symmetry == "C2_4"):
        cellrows = 40
    elif (symmetry == "C2_2"):
        cellrows = 40
    elif (symmetry == "C2_1"):
        cellrows = 40
    elif (symmetry == "C4_4"):
        cellrows = 40
    elif (symmetry == "C4_1"):
        cellrows = 40
    elif (symmetry == "D2_+2"):
        cellrows = 40
    elif (symmetry == "D2_+1"):
        cellrows = 40
    elif (symmetry == "D2_x"):
        cellrows = 32
    elif (symmetry == "D4_+4"):
        cellrows = 40
    elif (symmetry == "D4_+2"):
        cellrows = 40
    elif (symmetry == "D4_+1"):
        cellrows = 40
    elif (symmetry == "D4_x4"):
        cellrows = 40
    elif (symmetry == "D4_x1"):
        cellrows = 40
    elif (symmetry == "D8_4"):
        cellrows = 40
    elif (symmetry == "D8_1"):
        cellrows = 40
    else:
        print("Invalid symmetry: \033[1;31m"+symmetry+"\033[0m is not one of the supported symmetries:")
        print("    C1, C2_4, C2_2, C2_1, C4_4, C4_1,")
        print("    D2_+2, D2_+1, D2_x, D4_+4, D4_+2, D4_+1, D4_x4, D4_x1, D8_4, D8_1")
        exit(1)

    print("Valid symmetry: \033[1;32m"+symmetry+"\033[0m")

    (bee, ess) = binary_rulestring(rulestring)

    with open('includes/lifelogic-avx2.h', 'w') as g:
        genlogic(g, rulestring, 256)
    with open('includes/lifeasm-avx2.h', 'w') as g:
        genasm(g, cellrows / 8, rulestring, 256, False, 'avx2')
        genasm(g, cellrows / 8, rulestring, 256, True, 'avx2')

    with open('includes/lifelogic-avx1.h', 'w') as g:
        genlogic(g, rulestring, 192)
    with open('includes/lifeasm-avx1.h', 'w') as g:
        genasm(g, cellrows / 4, rulestring, 192, False, 'avx1')
        genasm(g, cellrows / 4, rulestring, 192, True, 'avx1')

    with open('includes/lifelogic-sse2.h', 'w') as g:
        genlogic(g, rulestring, 128)
    with open('includes/lifeasm-sse2.h', 'w') as g:
        genasm(g, cellrows / 4, rulestring, 128, False, 'sse2')
        genasm(g, cellrows / 4, rulestring, 128, True, 'sse2')

    with open('includes/params.h', 'w') as g:

        g.write('#define SYMMETRY "'+symmetry+'"\n')
        g.write('#define RULESTRING "'+rulestring+'"\n')
        g.write('#define RULESTRING_SLASHED "'+rulestring.replace('b', 'B').replace('s', '/S')+'"\n')
        g.write('#define BITTAGE 32\n')
        g.write('#define ROWS '+str(cellrows)+'\n')
        g.write('#define THSPACE 28\n')
        g.write('#define MIDDLE28 0x3ffffffcu\n')
        g.write('#define BIRTHS '+str(bee)+'\n')
        g.write('#define SURVIVALS '+str(ess)+'\n')
        g.write('typedef uint32_t urow_t;\n')
        if (rulestring == 'b3s23'):
            g.write('#define STANDARD_LIFE 1\n')
        if (re.match('b36?7?8?s0?235?6?7?8?$', rulestring)):
            g.write('#define GLIDERS_EXIST 1\n')


main()

print("Success!")
