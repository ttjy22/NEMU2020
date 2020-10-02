#include "monitor/monitor.h"
#include "cpu/helper.h"
#include <setjmp.h>

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INSTR_TO_PRINT 10

int nemu_state = STOP;

int exec(swaddr_t);

char assembly[80];
char asm_buf[128];

/* Used with exception handling. */
jmp_buf jbuf;

void print_bin_instr(swaddr_t eip, int len) {
    int i;
    int l = sprintf(asm_buf, "%8x:   ", eip);
    for (i = 0; i < len; i++) {
        l += sprintf(asm_buf + l, "%02x ", instr_fetch(eip + i, 1));
    }
    sprintf(asm_buf + l, "%*.s", 50 - (12 + 3 * len), "");
}

/* This function will be called when an `int3' instruction is being executed. */
void do_int3(int no) {
    printf("\nNo: %d Hit breakpoint at eip = 0x%08x\n", no, cpu.eip);
    nemu_state = STOP;
}

#include "monitor/watchpoint.h"

extern void ui_mainloop();

extern uint32_t expr(char *e, bool *success);

extern bool suc;

extern int count(char *args);

//extern void watchpoints();

WP *getHead();

/* Simulate how the CPU works. */
int cnt;

void cpu_exec(volatile uint32_t n) {
    if (nemu_state == END) {
        printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
        return;
    }
    nemu_state = RUNNING;

#ifdef DEBUG
    volatile uint32_t n_temp = n;
#endif

    setjmp(jbuf);

    for (; n > 0; n--) {
#ifdef DEBUG
        swaddr_t eip_temp = cpu.eip;
        if ((n & 0xffff) == 0) {
            /* Output some dots while executing the program. */
            fputc('.', stderr);
        }
#endif

        /* Execute one instruction, including instruction fetch,
         * instruction decode, and the actual execution. */
        int instr_len = exec(cpu.eip);

        cpu.eip += instr_len;

#ifdef DEBUG
        print_bin_instr(eip_temp, instr_len);
        strcat(asm_buf, assembly);
        Log_write("%s\n", asm_buf);
        if (n_temp < MAX_INSTR_TO_PRINT) {
            printf("%s\n", asm_buf);
        }
#endif

        /* TODO: check watchpoints here. */
        WP *head = getHead();
        while (head) {
//            watchpoints();
            int tp = count(head->express);
            if ((tp != head->res)) {
//                head->res = tp;
                do_int3(head->NO);
            }
//            printf("执行次数： %d\n ", ++cnt);
            head = head->next;
        }
#ifdef HAS_DEVICE
        extern void device_update();
        device_update();
#endif

        if (nemu_state != RUNNING) { return; }
    }

    if (nemu_state == RUNNING) { nemu_state = STOP; }
}
