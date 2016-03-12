#!/bin/bash
set -e

rulearg=`echo "$@" | grep -o "\\-\\-rule [a-z0-9]*" | sed "s/\\-\\-rule\\ //"`
updatearg=`echo "$@" | grep -o "\\-\\-update" | sed "s/\\-\\-update/u/"`

if ((${#updatearg} != 0))
then

printf "Checking for updates from repository...\033[30m\n"
newversion=`curl "https://gitlab.com/apgoucher/apgmera/raw/master/main.cpp" | grep "define APG_VERSION" | sed "s/#define APG_VERSION //"`
oldversion=`cat main.cpp | grep "define APG_VERSION" | sed "s/#define APG_VERSION //"`

if [ "$newversion" != "$oldversion" ]
then

printf "\033[0m...your copy of apgmera does not match the repository.\n"
echo "New version: $newversion"
echo "Old version: $oldversion"

git pull

else

printf "\033[0m...your copy of apgmera is already up-to-date.\n"

fi

else

echo "Skipping updates; use --update to update apgmera automatically."

fi

if ((${#rulearg} == 0))
then
rulearg="b3s23"
echo "Rule unspecified; assuming b3s23."
fi

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

exit 0
