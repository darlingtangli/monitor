#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <string>
#include "report.h"
#include "hash.h"

using namespace std;
using namespace inv::monitor;

int main(int argc, char* argv[])
{
    const int kCycles = 10000000;

    TIME_LABEL(1);
    for (int i = 0; i < kCycles; i++)
    {
        ReportCall("foo.bar.test_report_call",
                "",
                "bar",
                CS_SUCC,
                i);
    }
    cout << "ReportCall:    " << uint64_t(1000000)*kCycles/TIME_DIFF(1) << endl;

    TIME_LABEL(2);
    for (int i = 0; i < kCycles; i++)
    {
        ReportIncr("foo.bar.test_report_incr");
    }
    cout << "ReportIncr:    " << uint64_t(1000000)*kCycles/TIME_DIFF(2) << endl;

    TIME_LABEL(3);
    for (int i = 0; i < kCycles; i++)
    {
        ReportStatics("foo.bar.test_report_statics", i);
    }
    cout << "ReportStatics: " << uint64_t(1000000)*kCycles/TIME_DIFF(3) << endl;

    TIME_LABEL(4);
    for (int i = 0; i < kCycles; i++)
    {
        ReportAvg("foo.bar.test_report_avg", i);
    }
    cout << "ReportAvg:     " << uint64_t(1000000)*kCycles/TIME_DIFF(4) << endl;

    TIME_LABEL(5);
    for (int i = 0; i < kCycles; i++)
    {
        ReportMin("foo.bar.test_report_min", i);
    }
    cout << "ReportMin:     " << uint64_t(1000000)*kCycles/TIME_DIFF(5) << endl;

    TIME_LABEL(6);
    for (int i = 0; i < kCycles; i++)
    {
        ReportMax("foo.bar.test_report_max", i);
    }
    cout << "ReportMax:     " << uint64_t(1000000)*kCycles/TIME_DIFF(6) << endl;

    return 0;
}

