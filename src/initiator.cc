#include "initiator.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "shm_data.h"
#include "simple_spin_lock.h"
#include "hash.h"

namespace inv 
{

namespace monitor
{

const char*    VERSION      = "1.0";
const char*    MAGIC        = "MONITOR";
const uint32_t SHM_KEY      = 0xABCD0605;   
const uint32_t SHM_CAPCITY  = 10*1024*1024; // 10MB
const uint32_t OFFSET       = 1024;         // 上报数组偏移

Initiator::Initiator()
    : status(false), 
      shmaddr(NULL),
      instance_id(0),
      capcity(SHM_CAPCITY)
{
    // get process image path 
    char buf[256] = {'\0'};
    if (readlink ("/proc/self/exe", buf, sizeof(buf)) == -1) return;
    std::string path = buf;
    instance_id = BKDRHash(buf); // 路径名映射为整数标识
    if (!instance_id) return;
    base_name = path.substr(path.find_last_of('/')+1); 
    if (!base_name.length()) return;
    fprintf(stderr, "info: process image path: %s, base_name: %s, inst: %u\n", path.c_str(), base_name.c_str(), instance_id);
    
    int shmid = shmget(SHM_KEY, SHM_CAPCITY, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1 && errno != EEXIST) 
    {
        fprintf(stderr, "error: shmget failed.\n");
        return;
    }

    // 其他进程已经创建过共享内存
    if (errno == EEXIST)
    {
        // 打开已经存在的共享内存
        shmid = shmget(SHM_KEY, SHM_CAPCITY, 0666); 
        if (shmid == -1)
        {
            fprintf(stderr, "error: shmget failed, errno: %d.\n", errno);
            return;
        }
        shmaddr = shmat(shmid, NULL, 0);
        if (shmaddr == (void*)-1)
        {
            fprintf(stderr, "error: shmat failed, shmid: %d, errno: %d.\n", shmid, errno);
            return;
        }
        fprintf(stderr, "info: shm for monitor api already exist, id: %d.\n", shmid);
        if (GetHead(shmaddr)->version != MakeVersion(1, 0))
        {
            // api版本与共享内存上报数据版本不一致，打印警告信息
            fprintf(stderr, "warn: incompatible monitor version, api version: %s, "
                    "please remove shm for key 0x%X if neccesary.\n", VERSION, SHM_KEY);
        }
        status = true;
        return;
    }

    // 第一次创建本机上的上共享内存区
    shmaddr = (Head*)shmat(shmid, NULL, 0);
    if (shmaddr == (void*)-1)
    {
        fprintf(stderr, "error: shmat failed, shmid: %d, errno: %d.\n", shmid, errno);
        return;
    }
    Head* head = GetHead(shmaddr);
    fprintf(stderr, "info: shm for monitor api init for first time.\n");
    bzero(head, SHM_CAPCITY);
    strncpy((char*)head->magic, MAGIC, strlen(MAGIC));
    head->lock = SimpleSpinLock::UNLOCK;
    head->version = MakeVersion(1, 0);
    head->capcity = SHM_CAPCITY;
    head->offset = OFFSET;
    head->entries = 0;
    status = true;
    return;
}

Initiator::~Initiator()
{
    fprintf(stderr, "info: detach shm.\n");
    shmdt(shmaddr);
}

};

} // namespace monitor

