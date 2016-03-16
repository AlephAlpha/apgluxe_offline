//bin/cat /dev/null; outfile="$0.exe"
//bin/cat /dev/null; g++ -O3 -march=native -Wall -Wextra "$0" -o "$outfile"
//bin/cat /dev/null; "$outfile" "$@"
//bin/cat /dev/null; status=$?
//bin/cat /dev/null; rm "$outfile"
//bin/cat /dev/null; exit $status

#include "includes/vlife.h"

int main() {

    int i = testmain();
    return i;

}
