#ifndef FB_JOURNAL_H
#define FB_JOURNAL_H
#include "runtime.h"
#include <wchar.h>
typedef struct {
    uint64_t seq, time_ms, generation;
    unsigned site;
    FbStatus status;
    char reason[64];
} FbEvent;
typedef struct { uint64_t dropped, written, io_failures; bool enabled; } FbJournalStats;
bool fb_journal_start(const wchar_t *directory, FbError*);
/* Bounded, nonblocking, allocation-free. No filesystem work. */
bool fb_journal_enqueue(const FbEvent*);
/* Only the dedicated writer calls start/drain/stop. */
void fb_journal_drain(void);
void fb_journal_stop(void);
FbJournalStats fb_journal_stats(void);
#endif
