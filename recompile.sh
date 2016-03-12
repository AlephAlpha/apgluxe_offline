#!/bin/bash
set -e

if [[ "$1" == "--rule" ]]
then

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

echo "Configuring rule $2 for machine type $machine_type"

python rule2asm.py $2 $machine_type
make
./apgmera "$@"

else

echo "Usage: ./recompile.sh --rule b3s23 [ARGUMENTS]"

fi
