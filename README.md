监控API
====================
此API用于业务自定义指标监控，有C和C++两种风格的接口，详见头文件report.h。

系统需求
--------------------
操作系统：Linux

使用
--------------------
包含头文件**report.h**并链接库**libmonitor.a**

### 示例
监控接口RpcCallFoo的调用情况：
```cpp
    // test.cpp
    #include "report.h"
    
    using namespace inv::monitor;
    
    int main()
    {
        for (;;)
        {
            TIME_LABEL(1);                          // 标记调用开始的时间点1
            CallStatus status = CS_SUCC;
            try
            {
                bool ret = RpcCallFoo();
                if (!ret) status = CS_FAILED;       // 调用结果为失败
            }
            catch (...)
            {
                status = CS_EXCEPTION;              // 调用被异常中断
            }
            ReportCall("xz.gate.RpcCallFoo",        // 上报指标名
                    "",                             // 主调模块名，空串表示使用进程映像文件名
                    "Bar",                          // 被调用模块名"Bar"
                    status,                         // 调用结果
                    TIME_DIFF(1));                  // 与标记时间点1的时间差

        }

        for (int i = 0; i < 100; i++)
        {
            ReportIncr("xz.gate.xxxx.cycles");      // 上报递增量，指标"xz.gate.xxxx.cycles"值为100
        }
    
        return 0;
    }
```
编译指令：
    g++ ./test.cpp -I/usr/local/include -L/usr/local/lib -lmonitor

### 辅助工具
**ipcrm.sh** 删除监控上报使用的共享内存       
**mtool** 查看/清零共享内存中上报的监控数据     
**mreport** 用于shell脚本上报

性能
--------------------
### 时间性能
测试机环境：4核 1.80GHz CentOS操作系统(v6.5) metric长度32       
单进程测试每秒能调用监控接口的次数数据如下：

|接口|次数/秒|
|----|-------|
|ReportCall|3,153,759|
|ReportIncr|5,309,845|
|ReportStatics|5,482,438|
|ReportAvg|5,027,952|
|ReportMin|5,445,481|
|ReportMax|5,455,522|

### 内存占用
使用监控接口的机器上将占用10M的共享内存用于记录采集到的监控数据。

### FAQ
**1. 我在代码中插入监控API，会影响原有程序的性能吗？**    
不会。监控API所做的操作只是更改共享内存中的计数器变量，开销很小，单线程实测QPS在百万级别。      

**2. 监控API有没有BUG？会不会出现死循环？会不会影响我的业务进程？会不会导致我的程序coredump？**    
不会。监控API经过严格测试，没有coredump的BUG，没有死循环。监控API只是读写共享内存(key为0xabcd0605)的数据，在业务进程没有使用相同key的共享内存的情况下，不会影响业务进程。在最坏的情况下，如果监控API存在未发现的BUG，只会影响到监控数据上报的准确性，不会影响被监控进程原有的业务逻辑。监控API在平台部C++开发的模块中已经广泛使用，目前没有发现问题。        

**3. 监控API是否线程安全？**    
是线程安全的。     

**4. 监控API如何保证线程安全，有没有死锁？**   
监控API使用CPU的原子操作指令实现共享资源的读写，没有使用锁，不会出现死锁问题。   

**5. 监控API记录的数据准确吗？**    
监控API使用的计数器变量为64位整型，对于绝大多数正常的业务监控上报不会出现溢出情况。只要不是变态超级大的数据，监控API记录的数据是准确的。     

**6. 我在代码中上报的监控指标数有限制吗？**    
受限于共享内存的容量(10M)，目前单机最多只能上报4万个业务指标。如果超过了这个限制，API调用仍正常，但是上报的数据无法展示。     

**7. 监控指标命名有什么限制吗？**     
目前对监控指标的命名没有统一管理。调用方在保证唯一性的前提下可以随意命名(不超过64字节)。为了保证可读性并且不与其他指标命名冲突，建议使用用dot分隔的分层命名规则：
[业务名].[模块名].[上报属性名]...     

**8. ReportCall这个接口怎么这么多参数，能详细说明下使用场景吗？**      
ReportCall用来监控存在"调用"关系的场景，"调用"包括但不限于本地函数调用、远程过程调用、HTTP请求等。      
在同步调用模式下，接口使用比较简单。只需要在调用前记录一个时间戳，调用后记录一个时间戳，然后把两个时间戳的差值及调用结果传给ReportCall即可。在异步调用模式下，由于调用开始和结束可能不在同一个地方，要监控调用耗时可能需要一些特殊处理。例如在用libevhtp实现的HTTP Server上，要监控一个HTTP请求的处理耗时及处理结果，在收到HTTP请求时，可以把时间戳记录到request对象里：      
```cpp
    static void on_request(evhtp_request_t * request, void * arg)
    {
        request->timestamp = (void*)inv::monitor::Timer(); 
        // dispatch the request to process handle
        ...
    }
```
然后在回调的地方取出request里的时间戳并与当前时间计算时间差值：     
```cpp
    static void on_request_processed(evhtp_request_t * request, void * arg)
    {
        evhtp_send_reply(request, request->status); // 回包
        inv::monitor::ReportCall("yuntu.http.process.upload", 
            "", 
            "",
            request->status==EVHTP_RES_200?inv::monitor::CS_SUCC:inv::monitor::CS_FAILED,
            inv::monitor::Timer() - (uint64_t)request->timestamp);
    }
```

