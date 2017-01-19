#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "report.h"
#include "test_metrics.h"

using namespace std;
using namespace inv::monitor;

#define TEST_C_API 0

int test_metrics_size = 1000;

void* ThreadFunc(void* arg) {
    const int kCycles = 10000000;
    int tid = *(int*)arg;

    TIME_LABEL(1);
    for (int i = 0; i < kCycles; i++) {
#if TEST_C_API
        moni_report_incr(metrics[i%test_metrics_size], 1);
#else
        ReportIncr(metrics[i%test_metrics_size], 1);
#endif
    }
    fprintf(stderr, "tid: %d ReportIncr:    %lu\n", tid, uint64_t(1000000)*kCycles/TIME_DIFF(1));

    return NULL;
}

int main(int argc, char* argv[])
{
    int thread_num = 2;
    if (argc > 1) thread_num = atoi(argv[1]);
    if (argc > 2) test_metrics_size = atoi(argv[2]);
    
    pthread_t* threads = new pthread_t[thread_num];
    int* tids = new int[thread_num];
    for (int i = 0; i < thread_num; i++) {
        tids[i] = i;
        int ret = pthread_create(threads+i, NULL, ThreadFunc, tids+i);
        if (ret)
        {
            fprintf(stderr, "create thread %d failed, ret: %d\n", i, ret);
            exit(-1);
        }
    }

    for (int i = 0; i < thread_num; i++) {
        pthread_join(threads[i], NULL);
    }

    delete[] tids;
    delete[] threads;


}
