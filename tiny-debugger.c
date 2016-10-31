/*
 * The original author is Qi Yao. The code was listed in his book
 * <Debugger not in Depth>. I port it to amd64 version and
 * made a few modifications.
 * */
#include <sys/ptrace.h>
#include <errno.h>
#include <asm/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <unistd.h>
#include <stdio.h>

// dl_debug_state
//#define DL_ENTRY_ADDR 0x00007ffff7de7e90
// the address of foo() in int3func
#define DL_ENTRY_ADDR 0x40052a

union instr {
    long l;
    unsigned char c[sizeof(long)/sizeof(char)];

}u;

struct breakpoint {
    unsigned char orig;
    void *address;
};

#define DEBUG
//#undef DEBUG

extern pid_t child;

void setup_breakpoint(pid_t child, long address, struct breakpoint *b) {
    long align = (address >> 2) << 2;
    int idx = address % 4;

    u.l = ptrace(PTRACE_PEEKTEXT, child, align, NULL);
    if (errno > 0)
        printf("errno = %d\n", errno);

    b->address  = (void*) address;

#ifdef DEBUG
    printf("Get instr "
            "[0x%x 0x%x 0x%x 0x%x]"
            "[0x%x 0x%x 0x%x 0x%x]"
            "from 0x%lx\n",
            u.c[0], u.c[1], u.c[2], u.c[3],
            u.c[4], u.c[5], u.c[6], u.c[7],
            align);
#endif

    b->orig = u.c[idx];
    u.c[idx] = 0xcc;
    ptrace(PTRACE_POKETEXT, child, align, u.l);

#ifdef DEBUG
    printf("Set breakpoint at 0x%lx, and write "
            "[0x%x 0x%x 0x%x 0x%x]"
            "[0x%x 0x%x 0x%x 0x%x] at 0x%lx\n",
            address,
            u.c[0], u.c[1], u.c[2], u.c[3],
            u.c[4], u.c[5], u.c[6], u.c[7],
            align);
#endif
}

void restore_breakpoint(pid_t child, struct breakpoint *b) {
    long pc;
    long align = (long) b->address;
    int idx = align % 4;
    align = (align >> 2) << 2;

    u.l = ptrace(PTRACE_PEEKTEXT, child, align, NULL);

    u.c[idx] = b->orig;
    ptrace(PTRACE_POKETEXT, child, align, u.l);

#ifdef DEBUG
    printf("Restore the orginal instruction at 0x%lx\n",
            align);
#endif


}

void hit_breakpoint(pid_t child, struct breakpoint *b) {
    long pc;
    long align = (long) b->address;
    int idx = align % 4;
    align = (align >> 2) << 2;

    pc = ptrace(PTRACE_PEEKUSER, child, 8 * RIP, NULL);

#ifdef DEBUG
    printf("        PC = 0x%lx\n", pc);
    printf("b->address = 0x%lx\n", b->address);
#endif

    if (pc != (b->address) + 1) {
        printf("hit_breakpoint: pc != (b->address) + 1... RETURN NOW.\n");
        return;
    }

    u.l = ptrace(PTRACE_PEEKTEXT, child, align, NULL);
    u.c[idx] = b->orig;

    ptrace(PTRACE_POKETEXT, child, align, u.l);

#ifdef DEBUG

    printf("Hit breakpoint: Restore the original instr"
            "[0x%x 0x%x 0x%x 0x%x]"
            "[0x%x 0x%x 0x%x 0x%x] at 0x%lx\n",
            u.c[0], u.c[1], u.c[2], u.c[3],
            u.c[4], u.c[5], u.c[6], u.c[7],
            align);
#endif

    ptrace(PTRACE_POKEUSER, child, 8 * RIP, pc - 1);
    ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
    wait(NULL);

    pc = ptrace(PTRACE_PEEKUSER, child, 8 * RIP, NULL);

#ifdef DEBUG
    printf("Single Step: PC = 0x%lx\n", pc);
#endif
    b->orig = u.c[idx];
    u.c[idx] = 0xcc;
    ptrace(PTRACE_POKETEXT, child, align, u.l);


#ifdef DEBUG
    printf("Restore TRAP(0xCC) at 0x"
            "[0x%x 0x%x 0x%x 0x%x]"
            "[0x%x 0x%x 0x%x 0x%x] at 0x%lx\n",
            u.c[0], u.c[1], u.c[2], u.c[3],
            u.c[4], u.c[5], u.c[6], u.c[7],
            align);
#endif
}


int main() {
    pid_t child = fork();

    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        //execl("/bin/ls", NULL);
        execl("./int3func", NULL);
    } else {
        int status;
        long pc;
        struct breakpoint b;

        wait(NULL);

        setup_breakpoint(child, DL_ENTRY_ADDR, &b);
        ptrace(PTRACE_CONT, child, NULL, NULL);

        while (1) {
            wait(&status);

            if (WIFEXITED(status)) {
                printf("Exit status: %d\n", WEXITSTATUS(status));
                return 0;
            }
            if (WIFSTOPPED(status)) {
                printf("Stopped by: %d\n", WSTOPSIG(status));
                hit_breakpoint(child, &b);
                // 11 = SIGSEGV
                if (status == 11)
                    break;
            }
            if (WIFSIGNALED(status)) {
                printf("SIGNALED by: %d\n", WTERMSIG(status));
            }

            // Why is '8 * RIP' correct?
            pc = ptrace(PTRACE_PEEKUSER, child, 8 * RIP, NULL);
            printf("at 0x%lx\n", pc);
            ptrace(PTRACE_CONT, child, NULL, NULL);
        }
    }
    return 0;
}
