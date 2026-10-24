#include "journal.h"
#include <assert.h>
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
int main(int argc,char**argv){
    assert(argc==3);setlocale(LC_ALL,"");wchar_t path[2048];mbstowcs(path,argv[2],2048);
    FbError error={0};FbRuntime runtime;fb_runtime_init(&runtime,true);assert(fb_journal_start(path,&error));
    FbEvent event={.status=FB_OFF,.site=3};strcpy(event.reason,"quote\" and newline\n escaped");
    if(!strcmp(argv[1],"full")){
        signal(SIGXFSZ,SIG_IGN);struct rlimit limit={128,128};assert(!setrlimit(RLIMIT_FSIZE,&limit));
        for(unsigned i=0;i<100;i++)fb_journal_enqueue(&event);
        fb_journal_drain();FbJournalStats s=fb_journal_stats();assert(s.io_failures>0&&!s.enabled);
        assert(fb_runtime_status(&runtime)==FB_OFF);fb_journal_stop();return 0;
    }
    for(unsigned i=0;i<2000;i++)fb_journal_enqueue(&event);
    assert(fb_journal_stats().dropped>0);fb_journal_drain();
    for(unsigned i=0;i<60000;i++){event.time_ms=i;fb_journal_enqueue(&event);if(i%128==0)fb_journal_drain();}
    fb_journal_drain();assert(fb_journal_stats().written>30000);
    fb_journal_stop();assert(!fb_journal_enqueue(&event));assert(fb_runtime_status(&runtime)==FB_OFF);return 0;
}
