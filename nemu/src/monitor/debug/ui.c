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

typedef struct token {
    int type;
    char str[32];
} Token;
extern Token tokens[32];
extern int nr_token;
#define N 32
int stk_op[N], stk_n[N], t_op, t_n;
enum {
    NOT = 256, DEREF, EQ, NE, AND, OR, NOTYPE, VAL, HEC, REG

    /* TODO: Add more token types */

};

int getrank(int tp) {
    if (tp == '+')return 0;
    if (tp == '-')return 1;
    if (tp == '*')return 2;
    if (tp == '/')return 3;
    if (tp == OR)return 4;
    if (tp == AND)return 5;
    if (tp == EQ)return 6;
    if (tp == NE)return 7;
    if (tp == DEREF)return 8;
    if (tp == NOT)return 9;
    if (tp == HEC)return 10;
    return -1;
}

#define BIN_OP {int a = stk_n[t_n--], b = stk_n[t_n];if (tp == '+')stk_n[t_n] = a + b;if (tp == '-')stk_n[t_n] = b - a;if (tp == '*')stk_n[t_n] = a * b;if (tp == '/')stk_n[t_n] = b / a;if (tp == EQ){stk_n[t_n] = a == b;/*puts("dead loop!");*/}if (tp == NE)stk_n[t_n] = a != b;if (tp == AND)stk_n[t_n] = (a && b);if (tp == OR)stk_n[t_n] = (a || b);}

#include <string.h>
int cnt;
int count(char *args) {
    bool success = true;
    expr(args, &success);
    if (!success) return -1;
    for (int i = 0; i < nr_token; ++i) {
        if (tokens[i].type != VAL) {
            if (tokens[i].type == '(')stk_op[++t_op] = '(';
            else if (tokens[i].type == ')') {
                while (t_op && stk_op[t_op--] != '(') {
                    int tp = stk_op[t_op + 1];
//                    printf("stk_op:    %d\n", tp);
                    BIN_OP
                }
            } else {
                while (t_op && getrank(stk_op[t_op]) > getrank(tokens[i].type)) {
                    int tp = stk_op[t_op--];
//                    printf("stk_op:    %d\n", tp);
                    BIN_OP
                }
                int tp = tokens[i].type;
                if (tp == DEREF)stk_n[++t_n] = *(atoi(tokens[++i].str) + hw_mem);
                else if (tp == HEC) {
//                    puts("-----------HEC-----------");
                    int j = 0, base = 0;
                    i++;
                    for (; tokens[i].str[j]; ++j) {
                        if (tokens[i].str[j] >= 'a' && tokens[i].str[j] <= 'f')
                            base = tokens[i].str[j] - 'a' + 10 + base * 16;
                        else base = tokens[i].str[j] - '0' + base * 16;
                    }
                    stk_n[++t_n] = base;
                } else if (tp == REG) {
//                    puts("-----------REG-----------");
                    if (!strcmp("$eax", tokens[i].str))stk_n[++t_n] = cpu.eax;
                    if (!strcmp("$ecx", tokens[i].str))stk_n[++t_n] = cpu.ecx;
                    if (!strcmp("$edx", tokens[i].str))stk_n[++t_n] = cpu.edx;
                    if (!strcmp("$esp", tokens[i].str))stk_n[++t_n] = cpu.esp;
                    if (!strcmp("$ebp", tokens[i].str))stk_n[++t_n] = cpu.ebp;
                    if (!strcmp("$esi", tokens[i].str))stk_n[++t_n] = cpu.esi;
                    if (!strcmp("$ebx", tokens[i].str))stk_n[++t_n] = cpu.ebx;
                    if (!strcmp("$edi", tokens[i].str))stk_n[++t_n] = cpu.edi;
                    if (!strcmp("$eip", tokens[i].str))stk_n[++t_n] = cpu.eip;
//                    puts("----------");
//                    printf("%d\n", strcmp("$eip", tokens[i].str));
//                    printf("%s\n", tokens[i-1].str);
//                    printf("%d\n", stk_n[t_n]);
//                    puts("----------");
                } else if (tp == NOT) {
                    stk_n[++t_n] = !atoi(tokens[++i].str);
                } else {
                    stk_op[++t_op] = tp;//很多次
//                    printf("%d\n", tp==EQ);
//                    printf("%d\n", cnt++);
                }
            }
        } else {
            stk_n[++t_n] = atoi(tokens[i].str);
//            printf("stk_n:    %d\n", stk_n[t_n]);
        }
    }
//    printf("a\n");
    while (t_op) {//死循环了
//        printf("%d\n", t_op);
        int tp = stk_op[t_op--];
//        printf("t_op:      %d\n", t_op);//无穷大
//        printf("stk_op:    %d\n", tp);//非法操作数0不是258
        BIN_OP
//        printf("%d\n", t_op);
//        printf("stk_op:    %d\n", tp);
    }
//    printf("Done\n");
    int res = stk_n[t_n];
    t_n = 0;
    return res;
}

extern uint32_t expr(char *e, bool *success);

static int cmd_p(char *args) {
//    printf("0x%x\n", count(args));
    printf("%d\n", count(args));

    return 0;
}

extern WP *new_wp();

void ui_mainloop();

static int cmd_w(char *args) {
    WP *wp = new_wp();
    strcpy(wp->express, args);
    wp->res = count(wp->express);
    return 0;
}

extern void free_wp(int no);

static int cmd_d(char *args) {
    free_wp(atoi(args));
    return 0;
}
//static int cmd_bt(char *args){
//
//}

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

extern void watchpoints();
// TODO 记得改回来
int cmd_info(char *args) {
    if (!strcmp(args, "r")) {
        printf("eax : 0x%x\n", cpu.eax);
        printf("ecx : 0x%x\n", cpu.ecx);
        printf("edx : 0x%x\n", cpu.edx);
        printf("ebx : 0x%x\n", cpu.ebx);
        printf("esp : 0x%x\n", cpu.esp);
        printf("ebp : 0x%x\n", cpu.ebp);
        printf("esi : 0x%x\n", cpu.esi);
        printf("edi : 0x%x\n", cpu.edi);
        printf("eip : 0x%x\n", cpu.eip);
    }
    if (!strcmp(args, "w")) {
        watchpoints();
    }
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
        {"w",    "WATCH",                                             cmd_w},
        {"d",    "DELET WATCH POINT",                                 cmd_d},
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
