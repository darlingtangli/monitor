#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "report.h"

using namespace std;
using namespace inv::monitor;

#define TEST_C_API 0

void* ThreadFunc(void* arg)
{
    const int kCycles = 10000000;
    int tid = *(int*)arg;

    TIME_LABEL(1);
    for (int i = 0; i < kCycles; i++)
    {
#if TEST_C_API
        moni_report_call("foo.bar.test_report_call",
                "",
                "bar",
                MCS_SUCC,
                i);
#else
        ReportCall("foo.bar.test_report_call",
                "",
                "bar",
                CS_SUCC,
                i);
#endif
    }
    fprintf(stderr, "tid: %d ReportCall:    %lu\n", tid, uint64_t(1000000)*kCycles/TIME_DIFF(1));

    TIME_LABEL(2);
    for (int i = 0; i < kCycles; i++)
    {
#if TEST_C_API
        moni_report_incr("foo.bar.test_report_incr", 1);
#else
        ReportIncr("foo.bar.test_report_incr", 1);
#endif
    }
    fprintf(stderr, "tid: %d ReportIncr:    %lu\n", tid, uint64_t(1000000)*kCycles/TIME_DIFF(2));

    TIME_LABEL(3);
    for (int i = 0; i < kCycles; i++)
    {
#if TEST_C_API
        moni_report_statics("foo.bar.test_report_statics", i);
#else
        ReportStatics("foo.bar.test_report_statics", i);
#endif
    }
    fprintf(stderr, "tid: %d ReportStatics: %lu\n", tid, uint64_t(1000000)*kCycles/TIME_DIFF(3));

    TIME_LABEL(4);
    for (int i = 0; i < kCycles; i++)
    {
#if TEST_C_API
        moni_report_avg("foo.bar.test_report_avg", i);
#else
        ReportAvg("foo.bar.test_report_avg", i);
#endif
    }
    fprintf(stderr, "tid: %d ReportAvg:     %lu\n", tid, uint64_t(1000000)*kCycles/TIME_DIFF(4));

    TIME_LABEL(5);
    for (int i = 0; i < kCycles; i++)
    {
#if TEST_C_API
        moni_report_min("foo.bar.test_report_min", i);
#else
        ReportMin("foo.bar.test_report_min", i);
#endif
    }
    fprintf(stderr, "tid: %d ReportMin:     %lu\n", tid, uint64_t(1000000)*kCycles/TIME_DIFF(5));

    TIME_LABEL(6);
    for (int i = 0; i < kCycles; i++)
    {
#if TEST_C_API
        moni_report_max("foo.bar.test_report_max", i);
#else
        ReportMax("foo.bar.test_report_max", i);
#endif
    }
    fprintf(stderr, "tid: %d ReportMax:     %lu\n", tid, uint64_t(1000000)*kCycles/TIME_DIFF(6));

    return NULL;
}

int main(int argc, char* argv[])
{
    int thread_num = 2;
    if (argc > 1) thread_num = atoi(argv[1]);
    
    pthread_t* threads = new pthread_t[thread_num];
    int* tids = new int[thread_num];
    for (int i = 0; i < thread_num; i++)
    {
        tids[i] = i;
        int ret = pthread_create(threads+i, NULL, ThreadFunc, tids+i);
        if (ret)
        {
            fprintf(stderr, "create thread %d failed, ret: %d\n", i, ret);
            exit(-1);
        }
    }

    for (int i = 0; i < thread_num; i++)
    {
        pthread_join(threads[i], NULL);
    }

    delete[] tids;
    delete[] threads;

    return 0;
}
