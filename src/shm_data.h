/**
 * @file shm_data.h
 * @brief 共享内存数据结构
 * @author darlingtangli@gmail.com
 * @version 1.0
 * @date 2016-05-12
 */
#ifndef __SHM_DATA_H
#define __SHM_DATA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NAME_LEN 64

#pragma pack(1)

// 共享内存区头部信息
typedef struct moni_head_s {
    uint8_t  magic[8];  // magic number: 'MONITOR'
    uint8_t  lock;      // simple spin lock,用于增加entry时上锁
    uint16_t version;   // 版本号
    uint32_t capcity;   // 共享内存区大小
    uint32_t offset;    // 上报属性表偏移
    uint32_t entries;   // 上报属性数
} moni_head_t;

// 上报属性类型
typedef enum moni_attr_e {
    AT_CALL    = 0, // 接口调用
    AT_INCR    = 1, // 递增量
    AT_STATICS = 2, // 稳定量
    AT_AVG     = 3, // 平均值
    AT_MIN     = 4, // 最大值
    AT_MAX     = 5, // 最小值
} moni_attr_t;

typedef struct entry_s {
    union {
        ///////////////////// 上报属性数据区 /////////////////////////////
        struct {
            moni_attr_t type;
            uint32_t    instance_id;           // 上报的实例进程ID，和进程的映像文件的路径一一对应
            uint8_t     metric[MAX_NAME_LEN];  // 上报的指标名
            union {
                // AT_CALL
                struct {
                    uint8_t  caller[MAX_NAME_LEN]; // 主调服务名
                    uint8_t  callee[MAX_NAME_LEN]; // 被调服务名
                    uint32_t count;                // 调用总次数
                    uint32_t succ;                 // 调用返回成功量
                    uint32_t exception;            // 调用抛出异常量
                    uint64_t cost_us;              // 调用耗时累积(微秒)
                    uint32_t cost_min_us;          // 调用耗时最小值(微秒)
                    uint32_t cost_max_us;          // 调用耗时最大值(微秒)
                } call;
                // AT_INCR
                uint64_t incr;
                // AT_STATICS
                uint64_t statics;
                // AT_AVG
                struct {
                    uint64_t sum;
                    uint64_t count;
                } avg;
                // AT_MIN
                uint64_t min;
                // AT_MAX
                uint64_t max;
            } record;

        } data;
        ////////////////////////////////////////////////////////////////////

        uint8_t block[256]; // 用来保证entry大小正好是8的整数倍，注意根据前面上报的数据
                            // 调整这个值

    };
} moni_entry_t;

#pragma pack()

#define moni_make_version(high, low) (((uint16_t)high<<8)|low)

#define moni_get_head(addr) ((moni_head_t*)addr)

#define moni_get_entry(addr, index) (&((moni_entry_t*)((char*)addr+(((moni_head_t*)addr)->offset)))[index])

#ifdef __cplusplus
}
#endif

#endif // __SHM_DATA_H
