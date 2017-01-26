// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// process stat tools
//

#ifndef SRC_LDA_PROCESS_STAT_H_
#define SRC_LDA_PROCESS_STAT_H_

#include <stdint.h>

/*
 * return the CPU usage from previous call of GetCPUUsage to current call of GetCPUUsage
 * return value is in range [0, 100]
 * return -1 failure
 */
int GetCPUUsage();

/*
 * get the process' memory/virtual memory usage
 * return 0 OK/-1 failure
 */
int GetMemoryUsage(uint64_t* mem, uint64_t* vmem);

#endif  // SRC_LDA_PROCESS_STAT_H_
