CC=g++

# Thanks to Andrew Trevorrow for the following routine to handle clang:
ifeq "$(shell uname)" "Darwin"
    # we're on Mac OS X, so check if clang is available
    ifeq "$(shell which clang++)" "/usr/bin/clang++"
        # assume we're on Mac OS 10.9 or later
        MACOSX_109_OR_LATER=1
    endif
endif
ifdef MACOSX_109_OR_LATER
    # g++ is really clang++ and there is currently no OpenMP support
    CFLAGS=-c -Wall -O3 -march=native
    # CFLAGS=-c -Wall -Wextra -O3
else
    # assume we're using gcc with OpenMP support
    CFLAGS=-c -Wall -O3 -march=native -fopenmp -DUSE_OPEN_MP
    # CFLAGS=-c -Wall -Wextra -O3 -fopenmp -DUSE_OPEN_MP
    LDFLAGS=-fopenmp
endif

SOURCES=main.cpp includes/sha256.cpp includes/md5.cpp includes/happyhttp.cpp \
gollybase/bigint.cpp gollybase/lifealgo.cpp gollybase/qlifealgo.cpp gollybase/util.cpp \
gollybase/lifepoll.cpp gollybase/liferules.cpp gollybase/viewport.cpp \
gollybase/readpattern.cpp gollybase/qlifedraw.cpp

OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=apgmera

# Compile:
all: $(SOURCES) $(EXECUTABLE)
	true
	true                                                oo o
	true                                                oo ooo
	true                                                      o
	true                                                oo ooo
	true                                                 o o
	true                                                 o o
	true                                                  o

# Thanks to Apple Bottom for this routine to handle dependencies:
depend: .depend

.depend: $(SOURCES)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^>>./.depend;

# Clean the build environment by deleting any object files:
clean: 
	rm -f $(OBJECTS)
	rm -f ./.depend
	echo Clean done

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

include .depend

