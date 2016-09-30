#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include "shm_data.h"

using namespace std;
using namespace inv::monitor;

const char*    MAGIC        = "MONITOR";
const uint32_t SHM_KEY      = 0xABCD0605;   
const uint32_t SHM_CAPCITY  = 10*1024*1024; // 10MB

void Show(void* shmaddr)
{
    Head* head = GetHead(shmaddr);
    // find proper width for formatted output
    int width = 8;
    int metric_width = 8;
    int caller_width = 8;
    int callee_width = 8;
    int len = 0;
    for (uint32_t i = 0; i < head->entries; i++)
    {
        Entry* entry = GetEntry(shmaddr, i);
        len = strlen((char*)entry->data.metric);
        if (len > metric_width) metric_width = len;
        if (entry->data.type == AT_CALL)
        {
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
    for (uint32_t i = 0; i < head->entries; i++)
    {
        Entry* entry = GetEntry(shmaddr, i);

        switch (entry->data.type)
        {
            case AT_CALL:
                {
                    ss_call.flags(ios::left);
                    ss_call << setw(width) << i                       
                            << setw(width) << entry->data.type        
                            << setw(width+4) << entry->data.instance_id 
                            << setw(metric_width+1) << entry->data.metric
                            << setw(caller_width+1) << entry->data.record.call.caller
                            << setw(callee_width+1) << entry->data.record.call.callee
                            << setw(width)          << entry->data.record.call.count      
                            << setw(width)          << entry->data.record.call.succ       
                            << setw(width)          << entry->data.record.call.count-entry->data.record.call.succ-entry->data.record.call.exception       
                            << setw(width)          << entry->data.record.call.exception  
                            << setw(width+8)        << entry->data.record.call.cost_us    
                            << setw(width+1)        << entry->data.record.call.cost_min_us
                            << setw(width+1)        << entry->data.record.call.cost_max_us
                            << setw(width+1)        << entry->data.record.call.cost_us/max(uint32_t(1), entry->data.record.call.count)
                            << endl;
                    break;
                }
            case AT_INCR:
                {
                    ss_incr.flags(ios::left);
                    ss_incr << setw(width) << i                       
                            << setw(width) << entry->data.type        
                            << setw(width+4) << entry->data.instance_id 
                            << setw(metric_width+1) << entry->data.metric
                            << setw(width) << entry->data.record.incr
                            << endl;
                    break;
                }
            case AT_STATICS:
                {
                    ss_stat.flags(ios::left);
                    ss_stat << setw(width) << i                       
                            << setw(width) << entry->data.type        
                            << setw(width+4) << entry->data.instance_id 
                            << setw(metric_width+1) << entry->data.metric
                            << setw(width) << entry->data.record.statics
                            << endl;
                    break;
                }
            case AT_AVG:
                {
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
                }
            case AT_MIN:
                {
                    ss_min.flags(ios::left);
                    ss_min << setw(width) << i                       
                           << setw(width) << entry->data.type        
                           << setw(width+4) << entry->data.instance_id 
                           << setw(metric_width+1) << entry->data.metric
                           << setw(width) << entry->data.record.min
                           << endl;
                    break;
                }
            case AT_MAX:
                {
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
    }
    // output call
    if (ss_call.str().length())
    {
        cout << "----------- call data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(caller_width+1) << "caller"
             << setw(callee_width+1) << "callee" << setw(width) << "count" << setw(width) << "succ" 
             << setw(width) << "fail" << setw(width) << "ex" << setw(width+8) << "cost_us" << setw(width+1) << "min" 
             << setw(width+1) << "max" << setw(width+1) << "avg" << endl
             << ss_call.str() << endl;

    }
    // output incr
    if (ss_incr.str().length())
    {
        cout << "----------- incr data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(width+1) << "incr" << endl
             << ss_incr.str() << endl;
    }
    // output stat
    if (ss_stat.str().length())
    {
        cout << "----------- stat data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(width+1) << "statics" << endl
             << ss_stat.str() << endl;
    }
    // output avg
    if (ss_avg.str().length())
    {
        cout << "----------- avg  data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(width+13) << "sum" 
             << setw(width+13) << "count" << setw(width+13) << "avg" << endl
             << ss_avg.str() << endl;
    }
    // output min
    if (ss_min.str().length())
    {
        cout << "----------- min  data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(width+1) << "min" << endl
             << ss_min.str() << endl;
    }
    // output max
    if (ss_max.str().length())
    {
        cout << "----------- max  data ------------" << endl
             << setw(width) << "index" << setw(width) << "type" << setw(width+4) << "inst" 
             << setw(metric_width+1) << "metric" << setw(width+1) << "max" << endl
             << ss_max.str() << endl;
    }

    cout << "----------------------------------" << endl << endl;

    return;
}

void Clear(void* shmaddr)
{
    Head* head = GetHead(shmaddr);

    for (uint32_t i = 0; i < head->entries; i++)
    {
        Entry* entry = GetEntry(shmaddr, i);

        switch (entry->data.type)
        {
            case AT_CALL:
                {
                    entry->data.record.call.count     = 0;
                    entry->data.record.call.succ      = 0;
                    entry->data.record.call.exception = 0;
                    entry->data.record.call.cost_us = 0;
                    entry->data.record.call.cost_min_us = -1;
                    entry->data.record.call.cost_max_us = 0;
                    break;
                }
            case AT_INCR:
                {
                    entry->data.record.incr = 0;
                    break;
                }
            case AT_STATICS:
                {
                    entry->data.record.statics = 0;
                    break;
                }
            case AT_AVG:
                {
                    entry->data.record.avg.sum   = 0;
                    entry->data.record.avg.count = 0;
                    break;
                }
            case AT_MIN:
                {
                    entry->data.record.min = -1;
                    break;
                }
            case AT_MAX:
                {
                    entry->data.record.max = 0;
                    break;
                }
        }
    }
    return;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("usage: %s -[sc]\n", argv[0]);
        printf("  -s Show SHM data\n");
        printf("  -c Clear SHM data\n");
        return 0;
    }

    int shmid = shmget(SHM_KEY, SHM_CAPCITY, 0666); 
    if (-1 == shmid)
    {
        fprintf(stderr, "error: shmget failed, shmid: %d, errno: %d.\n", shmid, errno);
        return 0;
    }
    void* shmaddr = shmat(shmid, NULL, 0);
    if ((void*)-1 == shmaddr)
    {
        fprintf(stderr, "error: shmat failed, shmid: %d, errno: %d.\n", shmid, errno);
        return 0;
    }
    fprintf(stderr, "info: get shm for monitor api info succ, id: %d.\n", shmid);

    int opt;
    while ((opt = getopt(argc, argv, "sc")) != -1) 
    {
        switch (opt)
        {
            case 's':
                {
                    Show(shmaddr);
                    break;
                }
            case 'c':
                {
                    Clear(shmaddr);
                    break;
                }
        }
    }
    return 0;

    shmdt(shmaddr);

    return 0;
}

