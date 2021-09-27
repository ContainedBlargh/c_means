#include "fail.h"

void* callstack[1024];
void trace_stack()
{
    int frames = backtrace(callstack, 1024);
    char** strs = backtrace_symbols(callstack, frames);
    int i;
    fprintf(stderr, FAIL_RED);
    for (i = 0; i < frames; i++)
    {
        fprintf(stderr, "%s\n", strs[i]);
    }
    fprintf(stderr, FAIL_RESET);
    free(strs);
}

void fail()
{
    trace_stack();
    exit(EXIT_FAILURE);
}
