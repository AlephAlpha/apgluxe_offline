#!/bin/bash

if [ -d "lifelib/avxlife" ]
then
printf "\033[33;1mEnsuring lifelib is up-to-date...\033[0m\n"
rm -rf "lifelib/avxlife/lifelogic" | true
python lifelib/avxlife/rule2asm.py "b3s23" > /dev/null
else
printf "\033[33;1mDownloading lifelib...\033[0m\n"
fi

git submodule update --init

