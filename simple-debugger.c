/*
 * The original author is Qi Yao. The code was listed in his book
 * <Debugger not in Depth>. I port it to amd64 version and
 * made a few modifications.
 * */
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv) {
    pid_t child = fork();

    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(argv[1], argv[1], 0);
    } else {
        int status;
        long pc;
        while (1) {
            wait(&status);

            if (WIFEXITED(status)) {
                printf("Exit status: %d\n", WEXITSTATUS(status));
                return 0;
            }
            if (WIFSTOPPED(status)) {
                printf("Stopped by: %d\n", WSTOPSIG(status));
            }
            if (WIFSIGNALED(status)) {
                printf("SIGNALED by: %d\n", WTERMSIG(status));
            }

            pc = ptrace(PTRACE_PEEKUSER, child, 8 * RIP, NULL);
            printf("at 0x%lx\n", pc);
            ptrace(PTRACE_CONT, child, NULL, NULL);
        }
    }
    return 0;
}
