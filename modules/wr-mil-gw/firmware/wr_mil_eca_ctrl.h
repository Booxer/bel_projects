#ifndef WR_MIL_ECA_CTRL_H_
#define WR_MIL_ECA_CTRL_H_

#include <stdint.h>
#include "wr_mil_value64bit.h"

volatile uint32_t *ECACtrl_init();
void ECACtrl_getTAI(volatile uint32_t *eca, TAI_t *tai);

// keep processor in an idle loop that will end at a specified TAI value (that has to be in the future)
// arguments:
//    eca     : valid pointer to ECA registers as obtained by EcaCtrl_init()
//    stopTAI : the TAI value when the function should return
//
//    tai_now : the function reads the time from the ECA. This parameter will contain that time value
//              in case the caller also wants to know the current time.
// return value: 0 if the function returns at the specified stopTAI. The jitter of the return time 
//                 was measured and is < 120 ns
//               delay[ns] if the specified stopTAI was too soon. In this case, the function
//                 returns at a time after the specified stopTAI
//               0xffffffff in case the wait was aborted because of too long wait time
uint32_t wait_until_tai(volatile uint32_t *eca, uint64_t stopTAI, TAI_t *tai_now);


#endif
