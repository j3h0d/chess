#ifndef PERFORMANCE_COUNTERS_H
#define PERFORMANCE_COUNTERS_H

#include <stdint.h>
//{}[]
typedef struct {
    uint32_t cycles;
    uint32_t ins_ret;
    uint32_t mem_ins_ret;
    uint32_t ic_miss;
    uint32_t dc_miss;
    uint32_t ic_stall;
    uint32_t dc_stall;
    uint32_t hazard;
    uint32_t alu_stall;
} performance_t; //for all nine different tests

void performance_reset(void);
void performance_read(performance_t *p);

#endif
