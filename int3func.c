#include <stdio.h>

void foo() {
    printf("foo\n");
//    asm("int3");
}
int main () {
    int i;
    for (i = 0; i < 5; i++) {
        foo();
    }
    return 0;
}
