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
        WP *tmp = free_->next;
        free_->next = head;
        head = free_;
        free_ = tmp;
        return head;
    }
    assert(0);
    return NULL;
}

void free_wp(WP *wp) {
    WP *w = &wp_pool[wp->NO];
    if (free_) {
        w->next = free_;
        free_ = w;
    } else {
        free_ = w;
    }
    WP *h = head;
    if (h->NO == w->NO)head = head->next;
    else {
        while (h && h->next->NO != w->NO)h = h->next;
        h->next = h->next->next;
    }
}
