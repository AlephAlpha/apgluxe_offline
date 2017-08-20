#!/bin/bash

if [ -d "lifelib/avxlife" ]
then
printf "\033[33;1mEnsuring lifelib is up-to-date...\033[0m\n"
else
printf "\033[33;1mDownloading lifelib...\033[0m\n"
fi

rm -rf "lifelib/avxlife/lifelogic" | true
python lifelib/avxlife/rule2asm.py "b3s23" > /dev/null
git submodule update --init

