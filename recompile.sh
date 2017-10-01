#!/bin/bash
set -e

chmod 755 "recompile.sh"

# Ensures 'make' works properly:
rm -f ".depend" | true
rm -f "main.o" | true

# Ensures compilation will fail unless rule2asm succeeds:
rm -f "includes/params.h" | true

rulearg=`echo "$@" | grep -o "\\-\\-rule [a-z0-9-]*" | sed "s/\\-\\-rule\\ //"`
symmarg=`echo "$@" | grep -o "\\-\\-symmetry [a-zA-Z0-9_+]*" | sed "s/\\-\\-symmetry\\ //"`

# Ensure lifelib matches the version in the repository:
# bash update-lifelib.sh
rm -rf "lifelib/avxlife/lifelogic" | true

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
./apgluxe "$@"
else
./apgluxe --rule $rulearg --symmetry $symmarg
fi

exit 0
