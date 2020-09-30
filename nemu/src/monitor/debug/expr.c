#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
    NOT = 256, DEREF, EQ, NE, AND, OR, NOTYPE, VAL, HEC, REG

    /* TODO: Add more token types */

};

static struct rule {
    char *regex;
    int token_type;
} rules[] = {

        /* TODO: Add more rules.
         * Pay attention to the precedence level of different rules.
         */

        {" +",                                 NOTYPE},                // spaces
        {"\\+",                                '+'},                    // plus
        {"\\-",                                '-'},                    // plus
        {"\\*",                                '*'},                    // plus
        {"\\/",                                '/'},                    // plus
        {"\\(",                                '('},                    // plus
        {"\\)",                                ')'},                    // plus
        {"0x",                                 HEC},
        {"\\*",                                DEREF},                       // equal
        {"==",                                 EQ},                     // equal
        {"!=",                                 NE},
        {"!",                                  NOT},                     // equal
        {"&&",                                 AND},                     // equal
        {"\\|\\|",                             OR},
        {"\\$",                                REG},
        {"(0|-?[1-9|a-f][0-9|a-f]*|[a-z]{3})", VAL},
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
    int i;
    char error_msg[128];
    int ret;

    for (i = 0; i < NR_REGEX; i++) {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0) {
            regerror(ret, &re[i], error_msg, 128);
            Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}

typedef struct token {
    int type;
    char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0') {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++) {

            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                printf("%d", rules[i].token_type);
                if (rules[i].token_type == '-' && *(e + position + 1) - '0' >= 0 && *(e + position + 1) - '9' <= 0 &&
                    (!nr_token || tokens[nr_token - 1].type != VAL)) {
                    continue;
                }
                if (rules[i].token_type == '*' && (!nr_token || !strcmp(tokens[nr_token - 1].str, "OP"))) {
                    continue;
                }
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position,
                    substr_len, substr_len, substr_start);
//                i?printf("%d\n", tokens[i - 1].type):puts("");

                position += substr_len;

                /* TODO: Now a new token is recognized with rules[i]. Add codes
                 * to record the token in the array `tokens'. For certain types
                 * of tokens, some extra actions should be performed.
                 */

                switch (rules[i].token_type) {
                    case VAL:
                        tokens[nr_token].type = rules[i].token_type, strcpy(tokens[nr_token].str, " "), strncpy(
                                tokens[nr_token].str, substr_start,
                                substr_len);
                        break;
                    default:
//                        printf("%s", tokens[nr_token].str);
                        tokens[nr_token].type = rules[i].token_type;
                        if (tokens[nr_token].type != '(' && tokens[nr_token].type != ')' &&
                            tokens[nr_token].type != HEC && tokens[nr_token].type != REG &&
                            tokens[nr_token].type != NOTYPE) {
                            strcpy(tokens[nr_token].str, " "),strcpy(tokens[nr_token].str, "OP");
                        }
                }
//                Log("%d", tokens[nr_token].type);
//                Log("%s", tokens[nr_token].str);
                nr_token++;
                break;
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }

    return true;
}

uint32_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }

    /* TODO: Insert codes to evaluate the expression. */
    for (int i = 0; i < nr_token; ++i) {
        if (tokens[i].type == '*');
    }
    return 0;
}

