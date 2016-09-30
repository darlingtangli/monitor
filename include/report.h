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
#include <string>

namespace inv 
{

namespace monitor
{

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
enum CallStatus
{
    CS_SUCC      = 0,  // 调用返回且返回码为成功
    CS_FAILED    = 1,  // 调用返回且返回码为失败
    CS_EXCEPTION = 2   // 调用被异常中断
    // ...
};

/**
 * @brief 计时器
 *
 * @return 当前时间(单位微秒)
 */
uint64_t Timer();

#define TIME_LABEL(x) \
    uint64_t __INV_MONITOR_TIME_LABEL_##x = inv::monitor::Timer()

#define TIME_DIFF(x) \
    (inv::monitor::Timer() - __INV_MONITOR_TIME_LABEL_##x)

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
void ReportCall(const std::string& metric,  
        const std::string& caller,          
        const std::string& callee,          
        CallStatus status,                  
        uint64_t cost_us);                  

/**
 * @brief 上报递增量
 *        展示层将展示监控周期内(minute)所有上报数值的总和。
 *
 * @param [IN] metric 指标名(不超过64字节)
 * @param [IN] step   递增步长
 */
void ReportIncr(const std::string& metric, uint64_t step = 1);

/**
 * @brief 上报统计量
 *        展示层将展示监控周期内(minute)最后一次上报的数值。
 *
 * @param metric [IN] 指标名(不超过64字节)
 * @param value  [IN] 上报数值
 */
void ReportStatics(const std::string& metric, uint64_t value);

/**
 * @brief 上报平均值
 *        展示层将展示监控周期内(minute)所有上报数值的平均值。
 *
 * @param metric [IN] 指标名(不超过64字节)
 * @param value  [IN] 上报数值
 */
void ReportAvg(const std::string& metric, uint64_t value);

/**
 * @brief 上报最小值
 *        展示层将展示监控周期内(minute)所有上报数值的最小值。
 *
 * @param metric [IN] 指标名(不超过64字节)
 * @param value  [IN] 上报数值
 */
void ReportMin(const std::string& metric, uint64_t value);

/**
 * @brief 上报最大值
 *        展示层将展示监控周期内(minute)所有上报数值的最大值。
 *
 * @param metric [IN] 指标名(不超过64字节)
 * @param value  [IN] 上报数值
 */
void ReportMax(const std::string& metric, uint64_t value);

/**
 * @brief 将长字符串hash成短的字符串(最大长度为6的数字串)
 *        需要以caller-callee二元组区分指标时，由于指标命名的长度限制，使用此函数
 *        将caller-callee信息hash成较短的标识编码到指标名中。
 *
 * @param str [IN] 输入字符串
 *
 * @return hash后的数字串
 */
std::string SimpleHash(const std::string& str);

/**
 * @brief 获取调用监控接口的进程的映像文件名
 *
 * @return 进程映像文件名
 */
std::string ProcessImageName();

/**
 * @brief 获取上报接口版本信息
 *
 * @return 版本号
 */
std::string Version();

} // namespace monitor

} // namespace inv

#endif //  __REPORT_H
