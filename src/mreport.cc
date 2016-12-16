#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "report.h"

using namespace std;
using namespace inv::monitor;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage:\n");
        fprintf(stderr, "%s call    <metric> <caller> <callee> <status> <cost_us>\n", argv[0]);
        fprintf(stderr, "%s incr    <metric> <step>\n", argv[0]);
        fprintf(stderr, "%s statics <metric> <value>\n", argv[0]);
        fprintf(stderr, "%s avg     <metric> <value>\n", argv[0]);
        fprintf(stderr, "%s min     <metric> <value>\n", argv[0]);
        fprintf(stderr, "%s max     <metric> <value>\n", argv[0]);
        return 1;
    }

    CallStatus status[] = {CS_SUCC, CS_FAILED, CS_EXCEPTION};

    if (!strcasecmp(argv[1], "call"))
    {
        if (argc < 7) return 2;
        ReportCall(argv[2], argv[3], argv[4], status[atoi(argv[5])], atoll(argv[6]));
    }
    else if (!strcasecmp(argv[1], "incr"))
    {
        if (argc < 4) return 3;
        ReportIncr(argv[2], atoll(argv[3]));
    }
    else if (!strcasecmp(argv[1], "statics"))
    {
        if (argc < 4) return 4;
        ReportStatics(argv[2], atoll(argv[3]));
    }
    else if (!strcasecmp(argv[1], "avg"))
    {
        if (argc < 4) return 5;
        ReportAvg(argv[2], atoll(argv[3]));
    }
    else if (!strcasecmp(argv[1], "min"))
    {
        if (argc < 4) return 6;
        ReportMin(argv[2], atoll(argv[3]));
    }
    else if (!strcasecmp(argv[1], "max"))
    {
        if (argc < 4) return 7;
        ReportMax(argv[2], atoll(argv[3]));
    }
    else
    {
        return 8;
    }

    return 0;
}
