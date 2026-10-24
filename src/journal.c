#include "journal.h"
#include "file_io.h"
#include <inttypes.h>
#include <stdatomic.h>
#include <string.h>
#define CAPACITY 256u
#define FILE_LIMIT (4u * 1024u * 1024u)
static FbEvent queue[CAPACITY];
static unsigned head, count;
static uint64_t sequence;
static atomic_flag queue_lock = ATOMIC_FLAG_INIT;
static atomic_bool enabled;
static atomic_uint_fast64_t dropped, written, failures;
/* Writer-owned fields; callbacks never touch these. */
static FILE *output;
static wchar_t paths[2][32768];
static unsigned current;
static size_t bytes;
static void lock_queue(void) { while (atomic_flag_test_and_set(&queue_lock)) {} }
static void unlock_queue(void) { atomic_flag_clear(&queue_lock); }
static void io_failure(void) {
    atomic_store(&enabled, false);
    atomic_fetch_add(&failures, 1);
    if (output) { fclose(output); output = NULL; }
}
static bool open_current(void) {
    output = fb_file_open(paths[current], L"wb");
    bytes = 0;
    if (!output) { io_failure(); return false; }
    /* Observe a full disk on the write that fails, not a later close. */
    if (setvbuf(output, NULL, _IONBF, 0)) { io_failure(); return false; }
    return true;
}
bool fb_journal_start(const wchar_t *directory, FbError *error) {
    fb_journal_stop();
    atomic_store(&dropped, 0); atomic_store(&written, 0); atomic_store(&failures, 0);
    if (wcslen(directory) > 32700) { fb_error(error, 1, "journal path too long"); return false; }
    for (unsigned i = 0; i < 2; ++i) swprintf(paths[i], 32768, L"%ls/play_%u.jsonl", directory, i);
    current = 1;
    if (!open_current()) { fb_error(error, 2, "journal cannot be opened"); return false; }
    if (fclose(output)) { output = NULL; io_failure(); fb_error(error, 2, "journal cannot be closed"); return false; }
    output = NULL; current = 0;
    if (!open_current()) { fb_error(error, 2, "journal cannot be opened"); return false; }
    lock_queue(); head = count = 0; sequence = 0; atomic_store(&enabled, true); unlock_queue();
    return true;
}
bool fb_journal_enqueue(const FbEvent *event) {
    if (!event || !atomic_load(&enabled)) return false;
    if (atomic_flag_test_and_set(&queue_lock)) { atomic_fetch_add(&dropped, 1); return false; }
    if (!atomic_load(&enabled) || count == CAPACITY) {
        if (atomic_load(&enabled)) atomic_fetch_add(&dropped, 1);
        unlock_queue(); return false;
    }
    FbEvent *slot = &queue[(head + count) % CAPACITY];
    *slot = *event; slot->reason[sizeof slot->reason - 1] = 0; slot->seq = ++sequence; ++count;
    unlock_queue(); return true;
}
static void encode_reason(const char *src, char dst[385]) {
    static const char hex[] = "0123456789abcdef";
    size_t out = 0;
    for (size_t i = 0; src[i] && i < 63; ++i) {
        unsigned char c = (unsigned char)src[i];
        if (c == '"' || c == '\\') { dst[out++] = '\\'; dst[out++] = (char)c; }
        else if (c < 32 || c >= 127) {
            dst[out++] = '\\'; dst[out++] = 'u'; dst[out++] = '0'; dst[out++] = '0';
            dst[out++] = hex[c >> 4]; dst[out++] = hex[c & 15];
        } else dst[out++] = (char)c;
    }
    dst[out] = 0;
}
void fb_journal_drain(void) {
    /* Fixed budget: a producer cannot keep this call running indefinitely. */
    for (unsigned i = 0; i < CAPACITY && atomic_load(&enabled); ++i) {
        FbEvent event;
        lock_queue();
        if (!count) { unlock_queue(); break; }
        event = queue[head]; head = (head + 1) % CAPACITY; --count; unlock_queue();
        char escaped[385], line[768]; encode_reason(event.reason, escaped);
        int length = snprintf(line, sizeof line,
            "{\"seq\":%" PRIu64 ",\"time_ms\":%" PRIu64 ",\"status\":\"%s\",\"generation\":%" PRIu64
            ",\"site\":%u,\"reason\":\"%s\",\"dropped\":%" PRIuFAST64 "}\n",
            event.seq, event.time_ms, fb_status_name(event.status), event.generation, event.site, escaped, atomic_load(&dropped));
        if (length < 0 || (size_t)length >= sizeof line) { atomic_fetch_add(&dropped, 1); continue; }
        if (bytes + (size_t)length > FILE_LIMIT) {
            if (fclose(output)) { output = NULL; io_failure(); break; }
            output = NULL; current ^= 1u;
            if (!open_current()) break;
        }
        if (!output || fwrite(line, 1, (size_t)length, output) != (size_t)length) { io_failure(); break; }
        bytes += (size_t)length; atomic_fetch_add(&written, 1);
    }
}
void fb_journal_stop(void) {
    lock_queue(); atomic_store(&enabled, false); head = count = 0; unlock_queue();
    if (output) { if (fclose(output)) atomic_fetch_add(&failures, 1); output = NULL; }
}
FbJournalStats fb_journal_stats(void) {
    return (FbJournalStats){atomic_load(&dropped), atomic_load(&written), atomic_load(&failures), atomic_load(&enabled)};
}
