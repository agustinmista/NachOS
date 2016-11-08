#include "syscall.h"

int main () {
    int pid1 = Exec("../test/sort", 2, 0);
    int pid2 = Exec("../test/matmult", 6, 0);

    Join(pid1);
    Join(pid2);

    return 0;
}
