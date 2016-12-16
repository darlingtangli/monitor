/**
 * @file report.h
 * @brief 监控接口
 * @author litang
 * @version 1.0
 * @date 2016-05-12
 */
#ifndef __REPORT_H
#define __REPORT_H

#include <stdint.h>

#ifdef __cplusplus
#include <string>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * NOTE:
 * 1. metric命名要保证唯一性，同一个metric只能用于一种上报接口
 * 2. 为了确保metric命名唯一性，建议使用dot分隔的分层命名规则：
 *    [业务名].[模块名].[上报属性名]...
 *    eg: 'xiaozhi.gate.q.request' 上报'小知接入层q接口请求量'
 */

/**
 * @brief 接口调用结果状态码
 */
typedef enum moni_call_status_e {
    MCS_SUCC      = 0,  // 调用返回且返回码为成功
    MCS_FAILED    = 1,  // 调用返回且返回码为失败
    MCS_EXCEPTION = 2   // 调用被异常中断
    // ...
} moni_call_status_t;

/**
 * @brief 计时器
 *
 * @return 当前时间(单位微秒)
 */
uint64_t moni_timer();

#define TIME_LABEL(x) \
    uint64_t __INV_MONITOR_TIME_LABEL_##x = moni_timer()

#define TIME_DIFF(x) \
    (moni_timer() - __INV_MONITOR_TIME_LABEL_##x)

/**
 * @brief 上报接口调用信息
 *        展示层将展示接口调用信息，包括主/被调服务名，监控周期内(minute)调用总数、
 *        成功数、失败数、异常数、调用平均/最大/最小耗时等。
 *
 * @param metric  [IN] 指标名(不超过64字节)
 * @param caller  [IN] 主调服务名(不超过64字节)，空串""表示使用进程映像文件名
 * @param callee  [IN] 被调服务名(不超过64字节)
 * @param status  [IN] 调用结果
 * @param cost_us [IN] 调用耗时(微秒)           
 */
void moni_report_call(const char* metric,  
        const char* caller,          
        const char* callee,          
        moni_call_status_t status,                  
        uint64_t cost_us);                  

/**
 * @brief 上报递增量
 *        展示层将展示监控周期内(minute)所有上报数值的总和。
 *
 * @param [IN] metric 指标名(不超过64字节)
 * @param [IN] step   递增步长
 */
void moni_report_incr(const char* metric, uint64_t step);

/**
 * @brief 上报统计量
 *        展示层将展示监控周期内(minute)最后一次上报的数值。
 *
 * @param metric [IN] 指标名(不超过64字节)
 * @param value  [IN] 上报数值
 */
void moni_report_statics(const char* metric, uint64_t value);

/**
 * @brief 上报平均值
 *        展示层将展示监控周期内(minute)所有上报数值的平均值。
 *
 * @param metric [IN] 指标名(不超过64字节)
 * @param value  [IN] 上报数值
 */
void moni_report_avg(const char* metric, uint64_t value);

/**
 * @brief 上报最小值
 *        展示层将展示监控周期内(minute)所有上报数值的最小值。
 *
 * @param metric [IN] 指标名(不超过64字节)
 * @param value  [IN] 上报数值
 */
void moni_report_min(const char* metric, uint64_t value);

/**
 * @brief 上报最大值
 *        展示层将展示监控周期内(minute)所有上报数值的最大值。
 *
 * @param metric [IN] 指标名(不超过64字节)
 * @param value  [IN] 上报数值
 */
void moni_report_max(const char* metric, uint64_t value);

/**
 * @brief 将长字符串hash成短的字符串(最大长度为6的数字串)
 *        需要以caller-callee二元组区分指标时，由于指标命名的长度限制，使用此函数
 *        将caller-callee信息hash成较短的标识编码到指标名中。
 *
 * @param str [IN] 输入字符串
 * @param buf [OUT] 输出缓冲区
 * @param len [OUT] 输出缓冲区长度
 *
 * @return hash后的数字串
 */
void moni_simple_hash(const char* str, char* buf, int len);

/**
 * @brief 获取调用监控接口的进程的映像文件名
 *
 * @return 进程映像文件名
 */
const char* moni_process_image_name();

/**
 * @brief 获取上报接口版本信息
 *
 * @return 版本号
 */
const char* moni_version();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// 兼容老的C++接口

namespace inv 
{
namespace monitor
{

/**
 * NOTE:
 * 使用此类的目的：使用C++ API时如果传入的参数本身就是const char*类型，如果使 
 * const std::string&来接收，会产生string临时对象，经测试这种开销会导致性能降 
 * 一倍。在前述情形下，使用此类可以避免string临时对象的产生，同时仍然可以接收
 * string类型的参数
 */
class StringRef
{
public:
    StringRef(const char* str) : _cstr(str) {}
    StringRef(const std::string& str) : _string(str), _cstr(_string.c_str()) {}
    const char* c_str() const {return _cstr;}

private:
    std::string _string; // make sure the _cstr reference is still valid when call StringRef::c_str()
    const char* _cstr;

};

enum CallStatus {
    CS_SUCC      = 0,  // 调用返回且返回码为成功
    CS_FAILED    = 1,  // 调用返回且返回码为失败
    CS_EXCEPTION = 2   // 调用被异常中断
    // ...
};

inline uint64_t Timer()
{
    return moni_timer();
}

inline void ReportCall(const StringRef& metric,  
        const StringRef& caller,          
        const StringRef& callee,          
        CallStatus status,                  
        uint64_t cost_us) 
{
    return moni_report_call(metric.c_str(), caller.c_str(), callee.c_str(), 
            static_cast<moni_call_status_t>(status), cost_us);
}                 

inline void ReportIncr(const StringRef& metric, uint64_t step = 1)
{
    return moni_report_incr(metric.c_str(), step);
}

inline void ReportStatics(const StringRef& metric, uint64_t value)
{
    return moni_report_statics(metric.c_str(), value);
}

inline void ReportAvg(const StringRef& metric, uint64_t value)
{
    return moni_report_avg(metric.c_str(), value);
}

inline void ReportMin(const StringRef& metric, uint64_t value)
{
    return moni_report_min(metric.c_str(), value);
}

inline void ReportMax(const StringRef& metric, uint64_t value)
{
    return moni_report_max(metric.c_str(), value);
}

inline std::string SimpleHash(const std::string& str)
{
    char buf[32] = {'\0'};
    moni_simple_hash(str.c_str(), buf, sizeof(buf));
    return buf;
}

inline std::string ProcessImageName()
{
    return moni_process_image_name();
}

inline std::string Version()
{
    return moni_version();
}

} // namespace monitor
} // namespace inv

#endif  // __cplusplus

#endif //  __REPORT_H
