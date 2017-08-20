This program searches random initial configurations in Conway's Game
of Life and periodically uploads results to a remote server. You can
read more information about the distributed search at the following URL:

- http://catagolue.appspot.com/

An automatic live Twitter feed of new discoveries found by the search
was established by Ivan Fomichev:

- https://twitter.com/conwaylife

There is also an automatically-updated summary page, with animations
of interesting objects and various charts:

- http://catagolue.appspot.com/statistics

The search was originally performed by people running instances of
a Python script; this repository contains the source code for a C++
program which is 10 times faster. The prefix 'apg-' stands for _Ash
Pattern Generator_ and the suffix '-mera' refers to the chimeric
nature of the program (which recompiles itself whenever a different
rule is specified).

Compilation and execution
=========================

**Note:** apgmera can only run on **x86-64** machines. If you have an
ancient computer, then the recommended alternative is the Python
script.

This program is designed to be compiled using gcc, the Gnu Compiler
Collection. If you have this installed, then compiling apgmera is as
simple as running:

    bash recompile.sh

in the install directory. If compilation succeeded, the last two lines
should resemble the following:

    apgmera v4.03-ll1.0: Rule b3s23 is correctly configured.
    apgmera v4.03-ll1.0: Symmetry C1 is correctly configured.

which means you are ready to run the program like so:

    ./apgmera [OPTIONS]

The options may include, for example:

- **-k mypassword**      Upload soups where 'mypassword' is your key
- **-n 5000000**         Run 5000000 soups per upload
- **-p 4**               Parallelise across 4 threads
- **--rule b36s245**     Run the custom rule B36/S245
- **--symmetry D2_+1**   Run soups with odd bilateral symmetry

Example usage
-------------

This invocation will upload results every 20 million soups:

    ./apgmera -n 20000000

If you want to upload soups non-anonymously, use the -k flag and
provide a valid payosha256 key. The correct syntax is as follows:

    ./apgmera -n 20000000 -k mykey

where 'mykey' is replaced with your payosha256 key (available from
http://catagolue.appspot.com/payosha256 -- note the case-sensitivity).
Omitting this parameter will cause soups to be uploaded anonymously.

If you have a quad-core computer and would prefer not to run four
separate instances, then use the -p command to parallelise:

    ./apgmera -n 20000000 -k mykey -p 4

This will use OpenMP to parallelise across 4 threads, and thus will
produce and upload soups approximately four times more quickly.

Installation
============

Linux users
-----------

Compiling and running apgmera is easy, as explained above. To download
the source code, use the following command:

    git clone https://gitlab.com/apgoucher/apgmera.git

Then you can enter the directory and compile the search program using:

    cd apgmera
    ./recompile.sh

If the online repository is updated at all, you can update your local
copy in-place by running:

    ./recompile.sh --update

in the repository directory.

Windows users
-------------

Install Cygwin64 (from http://cygwin.com), ensuring that the following
are checked in the list of plugins to install:

 - git
 - make
 - gcc-g++
 - python2

Open a Cygwin terminal, which will behave identically to a Linux terminal
but run inside Windows. This reduces your problem to the above case.

If you get the error `stoll is not a member of std`, then you are using an
old version of GCC. Run the Cygwin setup program to ensure that gcc-g++ is
updated.

Mac OS X users
--------------

Again, use `./recompile.sh` to compile the executable. However, things are
more complicated:

Specifically, Mac OS X has clang (rather than gcc) as the default
compiler and symlinks calls to gcc components (such as g++) to redirect
to clang. Since clang lacks OpenMP support, this means that the -p flag
will be disabled by default when you compile on a Mac. To utilise all of
your cores, you will therefore need to run one instance per CPU core.

If you would prefer to have the full capabilities, download a copy of gcc
and replace g++ with g++-4.2.0 (or whatever your version is) at the
beginning of the makefile. You may also need to alter an environment
variable for the terminal to find g++.
