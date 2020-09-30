#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
    int i;
    for (i = 0; i < NR_WP; i++) {
        wp_pool[i].NO = i;
        wp_pool[i].next = &wp_pool[i + 1];
    }
    wp_pool[NR_WP - 1].next = NULL;

    head = NULL;
    free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP *new_wp() {
    if (free_) {
        WP *tp = head;
        head = free_;
        head->next = tp;
        free_ = free_->next;
        return head;
    }
    assert(0);
    return NULL;
}

#include <stdlib.h>

void free_wp(WP *wp) {
    WP *w = &wp_pool[wp->NO];
    if (free_) {
        w->next = free_;
        free_ = w;
    } else {
        free_ = w;
    }
    WP *h = head;
    if (h->NO == w->NO) {
        head = head->next;
        printf("%d\n", head->next->NO);
    } else {
        while (h && h->next->NO != w->NO)h = h->next;
        h->next = h->next->next;
    }
}

void watchpoints() {
    WP *h = head;
    puts("     Num       EXPRESSION ");
    while (h) {
        printf("      %d           %s\n", h->NO, h->express);
        h = h->next;
    }
}