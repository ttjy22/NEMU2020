#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char *rl_gets() {
    static char *line_read = NULL;

    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

    line_read = readline("(nemu) ");

    if (line_read && *line_read) {
        add_history(line_read);
    }

    return line_read;
}

static int cmd_c(char *args) {
    cpu_exec(-1);
    return 0;
}

static int cmd_q(char *args) {
    return -1;
}

static int cmd_si(char *args) {
    if (!args)cpu_exec(1);
    else cpu_exec(atoi(args));
    return 0;
}

static int cmd_help(char *args);

extern uint8_t *hw_mem;

static int cmd_x(char *args) {
    printf("%s\n", args);
    int step = 0, i, base = 0;
    printf("-----------------------------------\n");
    for (i = 0; args[i] != ' '; ++i) {
        step = args[i] - '0' + step * 10;
        printf("%d\n", step);
    }
    printf("-----------------------------------\n");
    i += 3;
    for (; args[i]; ++i) {
        if (args[i] >= 'a' && args[i] <= 'f')base = args[i] - 'a' + 10 + base * 16;
        else base = args[i] - '0' + base * 16;
        printf("%d\n", base);
    }
    printf("-----------------------------------\n");

    printf("%d\n", step);
    printf("%d\n", base);
    for (i = 0; i < step; ++i) {
        printf("%d\n", *(hw_mem + base + i * 4));
    }
    return 0;
}

extern CPU_state cpu;

static int cmd_info(char *args) {
    printf("eax : 0x%x\n", cpu.eax);
    printf("ecx : 0x%x\n", cpu.ecx);
    printf("edx : 0x%x\n", cpu.edx);
    printf("ebx : 0x%x\n", cpu.ebx);
    printf("esp : 0x%x\n", cpu.esp);
    printf("ebp : 0x%x\n", cpu.ebp);
    printf("esi : 0x%x\n", cpu.esi);
    printf("edi : 0x%x\n", cpu.edi);
    return 0;
}

static struct {
    char *name;
    char *description;

    int (*handler)(char *);
} cmd_table[] = {
        {"help", "Display informations about all supported commands", cmd_help},
        {"c",    "Continue the execution of the program",             cmd_c},
        {"q",    "Exit NEMU",                                         cmd_q},
        {"si",   "Step Into NEMU",                                    cmd_si},
        {"info", "print NEMU",                                        cmd_info},
        {"x",    "scan NEMU",                                         cmd_x},

        /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    int i;

    if (arg == NULL) {
        /* no argument given */
        for (i = 0; i < NR_CMD; i++) {
            printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    } else {
        for (i = 0; i < NR_CMD; i++) {
            if (strcmp(arg, cmd_table[i].name) == 0) {
                printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
                return 0;
            }
        }
        printf("Unknown command '%s'\n", arg);
    }
    return 0;
}

void ui_mainloop() {
    while (1) {
        char *str = rl_gets();
        char *str_end = str + strlen(str);

        /* extract the first token as the command */
        char *cmd = strtok(str, " ");
        if (cmd == NULL) { continue; }

        /* treat the remaining string as the arguments,
         * which may need further parsing
         */
        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end) {
            args = NULL;
        }

#ifdef HAS_DEVICE
        extern void sdl_clear_event_queue(void);
        sdl_clear_event_queue();
#endif

        int i;
        for (i = 0; i < NR_CMD; i++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
                if (cmd_table[i].handler(args) < 0) { return; }
                break;
            }
        }

        if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
    }
}
