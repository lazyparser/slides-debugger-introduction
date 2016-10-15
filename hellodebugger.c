#include <stdio.h>

int main() {
    printf("Hello, Debugger! Before in3.\n");
    asm("int3");
    printf("Hello, Debugger! After int3.\n");
    return 0;
}

