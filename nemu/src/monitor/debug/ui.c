#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

//#include <cstring.h>
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

typedef struct token {
    int type;
    char str[32];
} Token;
extern Token tokens[32];
extern int nr_token;
#define N 33
int stk_op[N], stk_n[N], t_op, t_n;
enum {
    NOT = 256, DEREF, EQ, NE, AND, OR, NOTYPE, NUM

    /* TODO: Add more token types */

};

int getrank(int tp) {
    if (tp == '+')return 0;
    if (tp == '-')return 1;
    if (tp == '*')return 2;
    if (tp == '/')return 3;
    if (tp == NOT)return 4;
    if (tp == DEREF)return 5;
    if (tp == EQ)return 6;
    if (tp == NE)return 7;
    if (tp == AND)return 8;
    if (tp == OR)return 9;
    return -1;
}

static int count() {
    for (int i = 0; i < nr_token; ++i) {
//        printf("%d\n", tokens[i].type == NOT);
        if (tokens[i].type < 256) {
            if (tokens[i].type == '(')stk_op[++t_op] = '(';
            else if (tokens[i].type == ')') {
                while (t_op && stk_op[t_op--] != '(') {
                    int tp = stk_op[t_op + 1];
//                    printf("stk_op:    %d\n", tp);
                    int a = stk_n[t_n], b = 1;
                    if (t_n)a = stk_n[t_n--], b = stk_n[t_n];
//                    if (tp == DEREF)stk_n[++t_n] = (*a);
                    if (tp == NOT){
                        printf("%d\n", atoi(tokens[++i].str));
                        stk_n[++t_n] = !atoi(tokens[++i].str);
                    }
                    if (tp == '+')stk_n[t_n] = a + b;
                    if (tp == '-')stk_n[t_n] = b - a;
                    if (tp == '*')stk_n[t_n] = a * b;
                    if (tp == '/')stk_n[t_n] = b / a;
                    if (tp == EQ)stk_n[t_n] = (a == b);
                    if (tp == NE)stk_n[t_n] = (a != b);
                    if (tp == AND)stk_n[t_n] = (a && b);
                    if (tp == OR)stk_n[t_n] = (a || b);
                }
            } else {
                while (t_op && getrank(stk_op[t_op]) > getrank(tokens[i].type)) {
                    int tp = stk_op[t_op--];
                    printf("stk_op:    %d\n", tp);
                    int a = stk_n[t_n], b = 1;
                    if (t_n)a = stk_n[t_n--], b = stk_n[t_n];
//                    if (tp == DEREF)stk_n[++t_n] = (*a);
                    if (tp == NOT){
                        printf("%d\n", atoi(tokens[++i].str));
                        stk_n[++t_n]=!atoi(tokens[++i].str);
                    }
                    if (tp == '+')stk_n[t_n] = a + b;
                    if (tp == '-')stk_n[t_n] = b - a;
                    if (tp == '*')stk_n[t_n] = a * b;
                    if (tp == '/')stk_n[t_n] = b / a;
                    if (tp == EQ)stk_n[t_n] = a == b;
                    if (tp == NE)stk_n[t_n] = a != b;
                    if (tp == AND)stk_n[t_n] = a && b;
                    if (tp == OR)stk_n[t_n] = a || b;
                }
                stk_op[++t_op] = tokens[i].type;
            }
        } else {
            stk_n[++t_n] = atoi(tokens[i].str);
//            printf("stk_n:    %d\n", stk_n[t_n]);
        }
    }
    while (t_op) {
        int tp = stk_op[t_op--];
//        printf("stk_op:    %d\n", tp);
        int a = stk_n[t_n], b = 1;
        if (t_n)a = stk_n[t_n--], b = stk_n[t_n];
//        if (tp == DEREF)stk_n[++t_n] = (*a);
        puts("here");
        if (tp == NOT){
            printf("%d\n", a);
            stk_n[++t_n] = !a;
        }
        if (tp == '+')stk_n[t_n] = a + b;
        if (tp == '-')stk_n[t_n] = b - a;
        if (tp == '*')stk_n[t_n] = a * b;
        if (tp == '/')stk_n[t_n] = b / a;
        if (tp == EQ)stk_n[t_n] = a == b;
        if (tp == NE)stk_n[t_n] = a != b;
        if (tp == AND)stk_n[t_n] = a && b;
        if (tp == OR)stk_n[t_n] = a || b;
    }
    return stk_n[t_n];
}

extern uint32_t expr(char *e, bool *success);

static int cmd_p(char *args) {
    bool success = true;
    expr(args, &success);
    if (success)printf("%d\n", count());
    return 0;
}

//static int cmd_help(char *args);
//static int cmd_help(char *args);
//static int cmd_help(char *args);

extern uint8_t *hw_mem;

static int cmd_x(char *args) {
    int step = 0, i, base = 0;
    for (i = 0; args[i] != ' '; ++i) {
        step = args[i] - '0' + step * 10;
    }
    i += 3;
    for (; args[i]; ++i) {
        if (args[i] >= 'a' && args[i] <= 'f')base = args[i] - 'a' + 10 + base * 16;
        else base = args[i] - '0' + base * 16;
    }
    for (i = 0; i < step; ++i) {
        printf("0x%x\n", *(hw_mem + base + i * 4));
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
        {"si",   "Step Into",                                         cmd_si},
        {"info", "print",                                             cmd_info},
        {"x",    "scan",                                              cmd_x},
        {"p",    "EXPR",                                              cmd_p},
//        {"w",    "WATCH",                                         cmd_w},
//        {"d",    "EXPR",                                         cmd_d},
//        {"bt",    "EXPR",                                         cmd_bt},

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
