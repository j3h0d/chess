#include "performance_counters.h"

//from the docs
void performance_reset(void) { 
    asm volatile("csrw mcycle, zero");
    asm volatile("csrw minstret, zero");
    asm volatile("csrw mhpmcounter3, zero");
    asm volatile("csrw mhpmcounter4, zero");
    asm volatile("csrw mhpmcounter5, zero");
    asm volatile("csrw mhpmcounter6, zero");
    asm volatile("csrw mhpmcounter7, zero");
    asm volatile("csrw mhpmcounter8, zero");
    asm volatile("csrw mhpmcounter9, zero");
}

void performance_read(performance_t *p) {
    asm volatile("csrr %0, mcycle"       : "=r"(p->cycles));
    asm volatile("csrr %0, minstret"     : "=r"(p->ins_ret));
    asm volatile("csrr %0, mhpmcounter3" : "=r"(p->mem_ins_ret));
    asm volatile("csrr %0, mhpmcounter4" : "=r"(p->ic_miss));
    asm volatile("csrr %0, mhpmcounter5" : "=r"(p->dc_miss));
    asm volatile("csrr %0, mhpmcounter6" : "=r"(p->ic_stall));
    asm volatile("csrr %0, mhpmcounter7" : "=r"(p->dc_stall));
    asm volatile("csrr %0, mhpmcounter8" : "=r"(p->hazard));
    asm volatile("csrr %0, mhpmcounter9" : "=r"(p->alu_stall));
}