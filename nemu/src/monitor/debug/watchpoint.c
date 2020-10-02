#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
    int i;
    for (i = 0; i < NR_WP - 1; i++) {
        wp_pool[i].NO = i;
        wp_pool[i].next = &wp_pool[i + 1];
    }
    wp_pool[NR_WP - 1].next = NULL;

    head = NULL;
    free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
void watchpoints() {
    WP *h = head;
    puts("     Num       EXPRESSION       RESULT");
    while (h) {
        printf("      %d           %s           %d\n", h->NO, h->express, h->res);
        h = h->next;
    }
}

WP *new_wp() {
    if (free_) {
        if (head) {
            WP *tp = head;
            head = free_;
            free_ = free_->next;
            head->next = tp;
        } else {
            head = free_;
            free_ = free_->next;
            head->next = NULL;
        }
        return head;
    }
    assert(0);
    return NULL;
}

#include <stdlib.h>

void free_wp(int no) {
    WP *w = &wp_pool[no], *h = head;
    if (h->NO == w->NO) {
        head = head->next;
//        printf("%d\n", head->next->NO);
    } else {
        while (h && h->next->NO != w->NO)h = h->next;
        h->next = h->next->next;
    }
    if (free_) {
        w->next = free_;
        free_ = w;
    } else {
        free_ = w;
        w->next = NULL;
    }
}


WP *getHead() {
    return head;
}