#include "kernel/timer.h"

#include "lib/ds/cbuffer.h"
#include "lib/elflist.h"

ELFLIST_DECLARE(timers);

struct event {
        callback_fn cb;
        unsigned wake_time;
};

static struct int_timer *TIMER;
static unsigned TIME;

static void callback(void)
{}

void timer_init(void)
{
        struct int_timer **t;
        ELFLIST_FOREACH (struct int_timer, timers, t) {
                if ((*t)->init != NULL) {
                        int rc = (*t)->init(callback);
                        if (rc != TIMER_RC_OK) {
                                continue;
                        }
                }
                TIMER = *t;
                break;
        }
}

void timer_call_after(unsigned ms, callback_fn f)
{
}
