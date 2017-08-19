#!/bin/bash
set -e

chmod 755 "recompile.sh"

if [ ! -d "lifelib/avxlife" ]
then
printf "\033[33;1mDownloading lifelib...\033[0m\n"
git submodule update --init
fi

# Ensures 'make' works properly:
rm -f ".depend" | true
rm -f "main.o" | true

# Ensures compilation will fail unless rule2asm succeeds:
rm -f "includes/params.h" | true
rm -rf "lifelib/avxlife/lifelogic" | true

rulearg=`echo "$@" | grep -o "\\-\\-rule [a-z0-9]*" | sed "s/\\-\\-rule\\ //"`
symmarg=`echo "$@" | grep -o "\\-\\-symmetry [A-Z0-9_+x]*" | sed "s/\\-\\-symmetry\\ //"`
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

launch=0

if ((${#rulearg} == 0))
then
rulearg="b3s23"
echo "Rule unspecified; assuming b3s23."
else
launch=1
fi

if ((${#symmarg} == 0))
then
symmarg="C1"
echo "Symmetry unspecified; assuming C1."
else
launch=1
fi

echo "Configuring rule $rulearg; symmetry $symmarg"

python lifelib/avxlife/rule2asm.py $rulearg
python mkparams.py $rulearg $symmarg
make

if (($launch == 1))
then
./apgmera "$@"
else
./apgmera --rule $rulearg --symmetry $symmarg
fi

exit 0
