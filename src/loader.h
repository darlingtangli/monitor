/**
 * @file loader.h
 * @brief api加载/卸载
 * @author litang
 * @version 1.0
 * @date 2016-05-13
 */
#ifndef __LOADER_H
#define __LOADER_H

#include "hash_map.h"

#define MAX_PATH_LEN 256

extern int             moni_status;
extern void*           moni_shmaddr;
extern int             moni_instance_id;
extern int             moni_capcity;
extern char            moni_base_name[MAX_PATH_LEN];
extern moni_hash_map_t moni_metric_index_map;

void __attribute__ ((constructor)) moni_load();
void __attribute__ ((destructor))  moni_unload();

#endif // __LOADER_H
