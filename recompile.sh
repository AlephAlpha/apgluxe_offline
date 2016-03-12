#!/bin/bash
set -e

rulearg=`echo "$@" | grep -o "\\-\\-rule [a-z0-9]*" | sed "s/\\-\\-rule\\ //"`

if ((${#rulearg} == 0))
then

echo "Usage: ./recompile.sh --rule b3s23 [ARGUMENTS]"
exit 1

else

echo $rulearg

nproc_avx2=`cat /proc/cpuinfo | grep flags | grep 'avx2' | wc -l`
nproc_avx=`cat /proc/cpuinfo | grep flags | grep 'avx' | wc -l`

if (($nproc_avx == 0))
then
machine_type="sse2"
elif (($nproc_avx2 == 0))
then
machine_type="avx1"
else
machine_type="avx2"
fi

echo "Configuring rule $rulearg for machine type $machine_type"

python rule2asm.py $rulearg $machine_type
make
./apgmera "$@"

fi

exit 0
