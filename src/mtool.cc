#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <set>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include "shm_data.h"

using namespace std;

const uint32_t SHM_KEY      = 0xABCD0605;   
const uint32_t SHM_CAPCITY  = 10*1024*1024; // 10MB

void ShowAll(void* shmaddr) {
    moni_head_t* head = moni_get_head(shmaddr);
    // find proper width for formatted output
    int width = 8;
    int metric_width = 8;
    int caller_width = 8;
    int callee_width = 8;
    int len = 0;
    for (uint32_t i = 0; i < head->entries; i++) {
        moni_entry_t* entry = moni_get_entry(shmaddr, i);
        len = strlen((char*)entry->data.metric);
        if (len > metric_width) metric_width = len;
        if (entry->data.type == AT_CALL) {
            len = strlen((char*)entry->data.record.call.caller);
            if (len > caller_width) caller_width = len;
            len = strlen((char*)entry->data.record.call.callee);
            if (len > callee_width) callee_width = len;
        }
        
    }

    // output head info
    cout.flags(ios::left);
    cout << "----------- Head Info ------------" << endl
         << "magic:   " << head->magic     << endl
         << "lock:    " << (int)head->lock << endl   
         << "version: " << (int)((char*)&head->version)[1] << "." 
                        << (int)((char*)&head->version)[0] << endl
         << "capcity: " << head->capcity/1024/1024 << "MB" << endl
         << "offset:  " << head->offset    << endl
         << "entries: " << head->entries   << endl
         << endl;

    // output entries info
    cout << "-----------  Entries  ------------" << endl;

    stringstream ss_call;
    stringstream ss_incr;
    stringstream ss_stat;
    stringstream ss_avg;
    stringstream ss_min;
    stringstream ss_max;
    for (uint32_t i = 0; i < head->entries; i++) {
        moni_entry_t* entry = moni_get_entry(shmaddr, i);

        switch (entry->data.type) {
            case AT_CALL: 
                ss_call.flags(ios::left);
                ss_call << setw(width) << i                       
                    << setw(width) << entry->data.type        
                    << setw(width+4) << entry->data.instance_id 
                    << setw(metric_width+1) << entry->data.metric
                    << setw(caller_width+1) << entry->data.record.call.caller
                    << setw(callee_width+1) << entry->data.record.call.callee
                    << setw(width+2)          << entry->data.record.call.count      
                    << setw(width+2)          << entry->data.record.call.succ       
                    << setw(width+2)          << entry->data.record.call.count-entry->data.record.call.succ-entry->data.record.call.exception       
                    << setw(width+2)          << entry->data.record.call.exception  
                    << setw(width+8)        << entry->data.record.call.cost_us    
                    << setw(width+1)        << entry->data.record.call.cost_min_us
                    << setw(width+1)        << entry->data.record.call.cost_max_us
                    << setw(width+1)        << entry->data.record.call.cost_us/max(uint32_t(1), entry->data.record.call.count)
                    << endl;
                break;
            case AT_INCR: 
                ss_incr.flags(ios::left);
                ss_incr << setw(width) << i                       
                    << setw(width) << entry->data.type        
                    << setw(width+4) << entry->data.instance_id 
                    << setw(metric_width+1) << entry->data.metric
                    << setw(width) << entry->data.record.incr
                    << endl;
                break;
            case AT_STATICS: 
                ss_stat.flags(ios::left);
                ss_stat << setw(width) << i                       
                    << setw(width) << entry->data.type        
                    << setw(width+4) << entry->data.instance_id 
                    << setw(metric_width+1) << entry->data.metric
                    << setw(width) << entry->data.record.statics
                    << endl;
                break;
            case AT_AVG: 
                ss_avg.flags(ios::left);
                ss_avg << setw(width) << i                       
                    << setw(width) << entry->data.type        
                    << setw(width+4) << entry->data.instance_id 
                    << setw(metric_width+1) << entry->data.metric
                    << setw(width+13) << entry->data.record.avg.sum   
                    << setw(width+13) << entry->data.record.avg.count 
                    << setw(width+13) << entry->data.record.avg.sum/max(uint64_t(1), entry->data.record.avg.count)
                    << endl;
                break;
            case AT_MIN: 
                ss_min.flags(ios::left);
                ss_min << setw(width) << i                       
                    << setw(width) << entry->data.type        
                    << setw(width+4) << entry->data.instance_id 
                    << setw(metric_width+1) << entry->data.metric
                    << setw(width) << entry->data.record.min
                    << endl;
                break;
            case AT_MAX: 
                ss_max.flags(ios::left);
                ss_max << setw(width) << i                       
                    << setw(width) << entry->data.type        
                    << setw(width+4) << entry->data.instance_id 
                    << setw(metric_width+1) << entry->data.metric
                    << setw(width) << entry->data.record.max
                    << endl;
                break;
        }
    }
    // output call
    if (ss_call.str().length()) {
        cout << "----------- call data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(caller_width+1) << "caller"
             << setw(callee_width+1) << "callee" << setw(width+2) << "count" << setw(width+2) << "succ" 
             << setw(width+2) << "fail" << setw(width+2) << "ex" << setw(width+8) << "cost_us" << setw(width+1) << "min" 
             << setw(width+1) << "max" << setw(width+1) << "avg" << endl
             << ss_call.str() << endl;

    }
    // output incr
    if (ss_incr.str().length()) {
        cout << "----------- incr data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(width+1) << "incr" << endl
             << ss_incr.str() << endl;
    }
    // output stat
    if (ss_stat.str().length()) {
        cout << "----------- stat data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(width+1) << "statics" << endl
             << ss_stat.str() << endl;
    }
    // output avg
    if (ss_avg.str().length()) {
        cout << "----------- avg  data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(width+13) << "sum" 
             << setw(width+13) << "count" << setw(width+13) << "avg" << endl
             << ss_avg.str() << endl;
    }
    // output min
    if (ss_min.str().length()) {
        cout << "----------- min  data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(width+1) << "min" << endl
             << ss_min.str() << endl;
    }
    // output max
    if (ss_max.str().length()) {
        cout << "----------- max  data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(width+1) << "max" << endl
             << ss_max.str() << endl;
    }

    cout << "----------------------------------" << endl << endl;

    return;
}

void ShowMetrics(void* shmaddr, const set<string>&metrics) {
    struct timeval now;
    gettimeofday(&now, NULL);
    now.tv_sec += 8*3600; // beijing timezone
    char timestamp[64] = "\0";
    snprintf(timestamp, sizeof(timestamp), "%ld:%ld:%ld.%ld", 
            now.tv_sec%86400/3600,
            now.tv_sec%3600/60,
            now.tv_sec%60, 
            now.tv_usec);
    
    moni_head_t* head = moni_get_head(shmaddr);
    for (uint32_t i = 0; i < head->entries; i++) {
        moni_entry_t* entry = moni_get_entry(shmaddr, i);
        if (metrics.find(reinterpret_cast<char*>(&entry->data.metric[0])) == metrics.end()) {
            continue;
        }

        cout << timestamp << " metric: " << entry->data.metric;
        switch (entry->data.type) {
            case AT_CALL: 
                cout << " type: call"
                    << " caller: " << entry->data.record.call.caller 
                    << " callee: " << entry->data.record.call.callee 
                    << " count: " << entry->data.record.call.count
                    << " succ: " << entry->data.record.call.succ
                    << " fail: " << entry->data.record.call.count-entry->data.record.call.succ-entry->data.record.call.exception
                    << " exception: " << entry->data.record.call.exception
                    << " avg_cost_usec: " << entry->data.record.call.cost_us/max(uint32_t(1), entry->data.record.call.count)
                    << endl;
                entry->data.record.call.count = 0;
                entry->data.record.call.succ = 0;
                entry->data.record.call.exception = 0;
                entry->data.record.call.cost_us = 0;
                entry->data.record.call.cost_min_us = 0;
                entry->data.record.call.cost_max_us = 0;
                break;
            case AT_INCR: 
                cout << " type: incr"
                    << "value: " << entry->data.record.incr
                    << endl;
                entry->data.record.incr = 0;
                break;
            case AT_STATICS: 
                cout << " type: stat"
                    << "value: " << entry->data.record.statics
                    << endl;
                entry->data.record.statics = 0;
                break;
            case AT_AVG: 
                cout << " type: avg"
                    << "value: " << entry->data.record.avg.sum/max(uint64_t(1), entry->data.record.avg.count)
                    << endl;
                entry->data.record.avg.sum = 0;
                entry->data.record.avg.count = 0;
                break;
            case AT_MIN: 
                cout << " type: min"
                    << "value: " << entry->data.record.min
                    << endl;
                entry->data.record.min = 0;
                break;
            case AT_MAX: 
                cout << " type: max"
                    << "value: " << entry->data.record.max
                    << endl;
                entry->data.record.max = 0;
                break;
        }
     }

    return;
}

void Clear(void* shmaddr) {
    moni_head_t* head = moni_get_head(shmaddr);

    for (uint32_t i = 0; i < head->entries; i++) {
        moni_entry_t* entry = moni_get_entry(shmaddr, i);

        switch (entry->data.type) {
            case AT_CALL: 
                entry->data.record.call.count     = 0;
                entry->data.record.call.succ      = 0;
                entry->data.record.call.exception = 0;
                entry->data.record.call.cost_us = 0;
                entry->data.record.call.cost_min_us = -1;
                entry->data.record.call.cost_max_us = 0;
                break;
            case AT_INCR: 
                entry->data.record.incr = 0;
                break;
            case AT_STATICS: 
                entry->data.record.statics = 0;
                break;
            case AT_AVG: 
                entry->data.record.avg.sum   = 0;
                entry->data.record.avg.count = 0;
                break;
            case AT_MIN: 
                entry->data.record.min = -1;
                break;
            case AT_MAX: 
                entry->data.record.max = 0;
                break;
        }
    }
    return;
}

void Scan(void* shmaddr, const set<string>& metrics, int interval_sec) {
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd < 0) {
        fprintf(stderr, "timerfd_create failed\n");
        exit(0);
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    struct itimerspec ts;
    ts.it_interval.tv_sec = interval_sec;
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = now.tv_sec + 1;
    ts.it_value.tv_nsec = 0;

    if (timerfd_settime(tfd, TFD_TIMER_ABSTIME, &ts, NULL) < 0) {
        fprintf(stderr, "timerfd_settime failed\n");
        close(tfd);
        exit(0);
    }

    int epfd = epoll_create(1);
    if (epfd == -1) {
        close(tfd);
        exit(0);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev) == -1) {
        close(epfd);
        close(tfd);
        exit(0);
    }

   while (epoll_wait(epfd, &ev, 1, -1) > 0) {
        time_t res;
        read(tfd, &res, sizeof(res));
        ShowMetrics(shmaddr, metrics);
    }
    printf("gracefully quit\n");

    close(epfd);
    close(tfd);

    return;
}

void* Attach() {
    int shmid = shmget(SHM_KEY, SHM_CAPCITY, 0666); 
    if (-1 == shmid) {
        fprintf(stderr, "error: shmget failed, shmid: %d, errno: %d.\n", shmid, errno);
        return NULL;
    }
    void* shmaddr = shmat(shmid, NULL, 0);
    if ((void*)-1 == shmaddr) {
        fprintf(stderr, "error: shmat failed, shmid: %d, errno: %d.\n", shmid, errno);
        return NULL;
    }
    fprintf(stderr, "info: get shm for monitor api info succ, id: %d.\n", shmid);
    return shmaddr;
}

void Detach(void* shmaddr) {
    if (shmaddr) {
        shmdt(shmaddr);
    }
}

const char* opts = "achs:i:";
const char* help = 
    "Options:\n"
    "  -h            : Show help\n"
    "  -a            : Show all shm data\n"
    "  -c            : Clear all shm data\n"
    "  -s <metric>   : Scan the metric by interval(default 1s)\n"
    "  -i <interval> : Scan interval(seconds)\n";

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("use %s -h to show usage.\n", argv[0]);
        return 0;
    }

    void* shmaddr;
    int i, opt;
    int interval_sec = 1;
    set<string> metrics;
    extern char* optarg;
    while ((opt = getopt(argc, argv, opts)) != -1) {
        switch (opt) {
            case 'h': 
                printf(help);
                return 0;
            case 'a': 
                if ((shmaddr=Attach()) != NULL) {
                    ShowAll(shmaddr);
                    Detach(shmaddr);
                }
                return 0;
            case 'c': 
                if ((shmaddr=Attach()) != NULL) {
                    Clear(shmaddr);
                    Detach(shmaddr);
                }
                return 0;
            case 'i': 
                i = atoi(optarg);
                if (i > 0) {
                    interval_sec = i;
                }
                break;
            case 's': 
                if (strlen(optarg) != 0) {
                    metrics.insert(optarg);
                }
                break;
        }
    }
    if (metrics.size() > 0) {
        if ((shmaddr=Attach()) != NULL) {
            Scan(shmaddr, metrics, interval_sec);
            Detach(shmaddr);
        }
    }

    return 0;
}
