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
    uint64_t start = Timer();
    //int cnt = 0;
    //while (cnt++<10)
    {
        for (int i = 0; i < 10000000; i++)
        {
            //ReportMax("ab012345678901234567890123456789", i);
            ReportCall("ab012345678901234567890123456789",
                    "",
                    "bar",
                    CS_SUCC,
                    i);
        }
        //sleep(1);
    }
    cout << Timer() - start << endl;

    return 0;
}

