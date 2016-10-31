#include <stdio.h>
int main() {
    printf("sizeof(long)= %lu, sizeof(size_t)= %lu, sizeof(long long)= %lu, sizeof(void*)= %lu\n",
            sizeof(long),
            sizeof(size_t),
            sizeof(long long),
            sizeof(void*));
return 0;
}
