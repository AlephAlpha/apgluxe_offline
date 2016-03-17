#!/bin/bash
set -e

chmod 777 "recompile.sh"

# Ensures 'make' works properly:
rm -f ".depend"

# Ensures compilation will fail unless rule2asm succeeds:
rm -f "includes/params.h"

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
launch=0
else
launch=1
fi

echo "Configuring rule $rulearg"

python rule2asm.py $rulearg
make

if (($launch == 1))
then
./apgmera "$@"
else
./apgmera --rule $rulearg
fi

exit 0
