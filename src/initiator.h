/**
 * @file initiator.h
 * @brief api初始化helper
 * @author litang
 * @version 1.0
 * @date 2016-05-13
 */
#ifndef __INITIATOR_H
#define __INITIATOR_H

#include <stdint.h>
#include <string>

namespace inv 
{

namespace monitor
{

class Initiator
{
public:
    Initiator();
    ~Initiator();

public:
    bool        status;       // 标识初始化结果
    void*       shmaddr;
    uint32_t    instance_id;
    uint32_t    capcity;
    std::string base_name;    // process image name

};

} // namespace monitor

} // namespace inv

#endif // __INITIATOR_H
