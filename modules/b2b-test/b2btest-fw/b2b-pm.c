/********************************************************************************************
 *  b2b-pm.c
 *
 *  created : 2019
 *  author  : Dietrich Beck, GSI-Darmstadt
 *  version : 27-May-2019
 *
 *  firmware required for measuring the h=1 phase for ring machine
 *  
 *  - when receiving B2BTEST_ECADO_PHASEMEAS, the phase is measured as a timestamp for an 
 *    arbitraty period
 *  - the phase timestamp is then sent as a timing message to the network
 *  
 * -------------------------------------------------------------------------------------------
 * License Agreement for this software:
 *
 * Copyright (C) 2018  Dietrich Beck
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstrasse 1
 * D-64291 Darmstadt
 * Germany
 *
 * Contact: d.beck@gsi.de
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * For all questions and ideas contact: d.beck@gsi.de
 * Last update: 15-April-2019
 ********************************************************************************************/
#define B2BPM_FW_VERSION 0x000005                                       // make this consistent with makefile

/* standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

/* includes specific for bel_projects */
#include "dbg.h"                                                        // debug outputs
#include "ebm.h"                                                        // EB master
#include "pp-printf.h"                                                  // print
#include "mini_sdb.h"                                                   // sdb stuff
#include "syscon.h"                                                     // usleep et al
#include "aux.h"                                                        // cpu and IRQ
#include "uart.h"                                                       // WR console

/* includes for this project */
#include <b2b-common.h>                                                 // common stuff for b2b
#include <b2b-test.h>                                                   // defs
#include <b2bpm_shared_mmap.h>                                          // autogenerated upon building firmware

// stuff required for environment
extern uint32_t* _startshared[];
unsigned int     cpuId, cpuQty;
#define  SHARED  __attribute__((section(".shared")))
uint64_t SHARED  dummy = 0;

// global variables 
volatile uint32_t *pShared;             // pointer to begin of shared memory region
uint32_t *pSharedNTransfer;             // pointer to a "user defined" u32 register; here: # of transfers
volatile uint32_t *pSharedTH1ExtHi;     // pointer to a "user defined" u32 register; here: period of h=1 extraction, high bits
volatile uint32_t *pSharedTH1ExtLo;     // pointer to a "user defined" u32 register; here: period of h=1 extraction, low bits
volatile uint32_t *pSharedTH1InjHi;     // pointer to a "user defined" u32 register; here: period of h=1 injection, high bits
volatile uint32_t *pSharedTH1InjLo;     // pointer to a "user defined" u32 register; here: period of h=1 injecion, low bits

uint32_t *pCpuRamExternal;              // external address (seen from host bridge) of this CPU's RAM            
uint32_t *pCpuRamExternalData4EB;       // external address (seen from host bridge) of this CPU's RAM: field for EB return values
uint32_t sumStatus;                     // all status infos are ORed bit-wise into sum status, sum status is then published
uint32_t nTransfer;                     // # of transfers

// for phase measurement
#define NSAMPLES 8                      // # of timestamps for sampling h=1
uint64_t tStamp[NSAMPLES];              // timestamp samples

void init() // typical init for lm32
{
  discoverPeriphery();        // mini-sdb ...
  uart_init_hw();             // needed by WR console   
  cpuId = getCpuIdx();

  timer_init(1);              // needed by usleep_init() 
  usleep_init();              // needed for usleep ...
} // init


void initSharedMem() // determine address and clear shared mem
{
  uint32_t idx;
  uint32_t *pSharedTemp;
  int      i; 
  const uint32_t c_Max_Rams = 10;
  sdb_location found_sdb[c_Max_Rams];
  sdb_location found_clu;
  
  // get pointer to shared memory
  pShared           = (uint32_t *)_startshared;

  // get address to data
  pSharedNTransfer        = (uint32_t *)(pShared + (B2BTEST_SHARED_NTRANSFER >> 2));
  pSharedTH1ExtHi         = (uint32_t *)(pShared + (B2BTEST_SHARED_TH1EXTHI  >> 2));
  pSharedTH1ExtLo         = (uint32_t *)(pShared + (B2BTEST_SHARED_TH1EXTLO  >> 2));
  pSharedTH1InjHi         = (uint32_t *)(pShared + (B2BTEST_SHARED_TH1INJHI  >> 2));
  pSharedTH1InjLo         = (uint32_t *)(pShared + (B2BTEST_SHARED_TH1INJLO  >> 2));
  
  // find address of CPU from external perspective
  idx = 0;
  find_device_multi(&found_clu, &idx, 1, GSI, LM32_CB_CLUSTER);	
  idx = 0;
  find_device_multi_in_subtree(&found_clu, &found_sdb[0], &idx, c_Max_Rams, GSI, LM32_RAM_USER);
  if(idx >= cpuId) {
    pCpuRamExternal           = (uint32_t *)(getSdbAdr(&found_sdb[cpuId]) & 0x7FFFFFFF); // CPU sees the 'world' under 0x8..., remove that bit to get host bridge perspective
    pCpuRamExternalData4EB    = (uint32_t *)(pCpuRamExternal + ((COMMON_SHARED_DATA_4EB + SHARED_OFFS) >> 2));
  }

  DBPRINT2("b2b-pm: CPU RAM External 0x%08x, begin shared 0x%08x\n", (unsigned int)pCpuRamExternal, (unsigned int)SHARED_OFFS);

  // clear shared mem
  i = 0;
  pSharedTemp        = (uint32_t *)(pShared + (COMMON_SHARED_BEGIN >> 2 ));
  while (pSharedTemp < (uint32_t *)(pShared + (B2BTEST_SHARED_END >> 2 ))) {
    *pSharedTemp = 0x0;
    pSharedTemp++;
    i++;
  } // while pSharedTemp
  DBPRINT2("b2b-pm: used size of shared mem is %d words (uint32_t), begin %x, end %x\n", i, (unsigned int)pShared, (unsigned int)pSharedTemp-1);

} // initSharedMem 


// clear project specific diagnostics
void extern_clearDiag()
{
  sumStatus = 0;
  nTransfer = 0;
} // extern_clearDiag
  

uint32_t extern_entryActionConfigured()
{
  uint32_t status = COMMON_STATUS_OK;

  // configure EB master (SRC and DST MAC/IP are set from host)
  if ((status = common_ebmInit(2000, 0xffffffffffff, 0xffffffff, EBM_NOREPLY)) != COMMON_STATUS_OK) {
    DBPRINT1("b2b-pm: ERROR - init of EB master failed! %u\n", (unsigned int)status);
    return status;
  } 

  // get and publish NIC data
  common_publishNICData();

  return status;
} // extern_entryActionConfigured


uint32_t extern_entryActionOperation()
{
  int      i;
  uint64_t tDummy;
  uint64_t pDummy;
  uint32_t flagDummy;

  // clear diagnostics
  common_clearDiag();             

  // flush ECA queue for lm32
  i = 0;
  while (common_wait4ECAEvent(1, &tDummy, &pDummy, &flagDummy) !=  COMMON_ECADO_TIMEOUT) {i++;}
  DBPRINT1("b2b-pm: ECA queue flushed - removed %d pending entries from ECA queue\n", i);

  return COMMON_STATUS_OK;
} // extern_entryActionOperation


uint32_t extern_exitActionOperation()
{
  return COMMON_STATUS_OK;
} // extern_exitActionOperation


uint32_t poorMansFit(uint64_t period, uint32_t nSamples, uint64_t *phase)
{
#define MATCHWINDOW 5    // samples are only accepted, if they are within this window [ns]
#define FITRANGE    2    // use this value for 'fitting' [ns]

  int64_t  delta;        // diff between timestamp and tTmp
  int64_t  sumDelta;     // sum of all 'valid' delta
  int64_t  sumDeltaMin;  // min sumDelta
  uint64_t t0;           // start timestamp
  uint64_t tInit;        // initial guess of fit
  uint64_t tTheo;        // expected time for timestamp
  uint64_t trickPeriod;  // scaled value of period (to ease division by 1e9)
  uint64_t tFit;         // fitted timestamp
  uint32_t nUsed;        // # of used timestamps for sumDelta
  uint32_t nUsedFit;     // # of used timestamps for sumDelta of fit
  int      h,i,j;

  uint64_t t1,t2;

  t1 = getSysTime();

  if ((nSamples < 2) || (nSamples > NSAMPLES)) return B2BTEST_STATUS_PHASEFAILED; // we need at least two samples (otherwise tStamp[1] is invalid)

  // the following algorithm is applied
  // - don't use the first sample at we don't know when exactly the input
  //   gate became active
  // - use the second sample to start with ( ~'t0')
  // samples may not be ordered, calculate overal deviation:
  // - three nested loops
  // -- loop: t0 is changed within +/- FITRANGE
  // -- loop: starting from t0 to nSamples * period
  // -- loop: over all samples: identify matches and calc deviation
  // note1: this is based on time [ns]. Higher precision will be achieved if moving to [ps].
  // note2: period is [as] (required for precise propagation into the future) 
  // note3: using '2' as FITRANGE and '8' as NSAMPLES, this routine takes about 65us
  
  sumDeltaMin = FITRANGE * NSAMPLES;
  tFit        = 0x0;
  tInit       = tStamp[1];
  nUsedFit    = 0;
  trickPeriod = (uint64_t)((double)period * 1.073741824);              // this allows using '>> 30' instead of '/ 1000000000'

  for (h = -FITRANGE; h <= +FITRANGE; h++) {                           // loop t0 over 'fit range'
    sumDelta = 0;
    nUsed    = 0;
    t0       = tInit + h;                                              // use 2nd sample
    for (i=1; i<nSamples; i++) {                                       // loop over expected timestamps (starting at t0)
      tTheo = t0 + ((((uint64_t)(i-1)) * trickPeriod) >> 30);          // expected time; note that trickPeriod is in [as]; use '>> 30' for conversion to [ns]
      for (j=1; j<nSamples; j++) {                                     // loop over measured samples
        delta = tStamp[j] - tTheo;                                     // decide whether actual sample is useful
        if (abs(delta) < MATCHWINDOW){
          nUsed++;
          sumDelta += delta;                                           // calculate sum deviation for actual 't0'
        } // if tStamp
      } // for j
    } // for i
    if (abs(sumDelta) < abs(sumDeltaMin)) {                            // check, if actual 't0' is best
      sumDeltaMin = sumDelta;
      tFit        = t0;
      nUsedFit    = nUsed;
    } // if sumDeltaMax
  } // for h

  t2 = getSysTime();
  DBPRINT2("b2b-pm: sumDeltaMin %d, nUsedFit %d, tFit - tStamp[1] %d, time for fit %u\n", (int)sumDeltaMin, (int)nUsedFit, (int)(tFit - tStamp[1]), (uint32_t)(t2-t1));

  if (tFit == 0x0)               return B2BTEST_STATUS_PHASEFAILED;    // fit failed entirely
  if (nUsedFit < (nSamples / 2)) return B2BTEST_STATUS_PHASEFAILED;    // at least half of the samples must match; if not s.th. is wrong
  if (sumDeltaMin > FITRANGE)    return B2BTEST_STATUS_PHASEFAILED;    // the sum of deviations must be small; FITRANGE is used as a measure

  *phase = tFit;

  return COMMON_STATUS_OK;
} //poorMansFit


uint32_t doActionOperation(uint64_t *tAct,                    // actual time
                           uint32_t actStatus)                // actual status of firmware
{
  uint32_t status;                                            // status returned by routines
  uint32_t flagIsLate;                                        // flag indicating that we received a 'late' event from ECA
  uint32_t ecaAction;                                         // action triggered by event received from ECA
  uint64_t recDeadline;                                       // deadline received
  uint64_t recParam;                                          // param received
  uint64_t sendDeadline;                                      // deadline to send
  uint64_t sendEvtId;                                         // evtid to send
  uint64_t sendParam;                                         // parameter to send
  
  int      nInput;                                            // # of timestamps
  uint64_t TH1;                                               // h=1 period
  uint64_t tH1Ext;                                            // h=1 timestamp of extraction ( = 'phase')
  uint64_t tH1Inj;                                            // h=1 timestamp of injection ( = 'phase')

  status = actStatus;

  ecaAction = common_wait4ECAEvent(COMMON_ECATIMEOUT, &recDeadline, &TH1, &flagIsLate);
  
  switch (ecaAction) {
    case B2BTEST_ECADO_B2B_PMEXT :
      *pSharedTH1ExtHi = (uint32_t)((TH1 >> 32)    & 0xffffffff);
      *pSharedTH1ExtLo = (uint32_t)( TH1           & 0xffffffff);
      
      nInput = 0;
      common_ioCtrlSetGate(1, 2);                                      // enable input gate
      while (nInput < NSAMPLES) {                                      // treat 1st TS as junk
        ecaAction = common_wait4ECAEvent(100, &recDeadline, &recParam, &flagIsLate);
        if (ecaAction == B2BTEST_ECADO_TLUINPUT)  {tStamp[nInput] = recDeadline; nInput++;}
        if (ecaAction == B2BTEST_ECADO_TIMEOUT)   break; 
      } // while nInput
      common_ioCtrlSetGate(0, 2);                                      // disable input gate 

      DBPRINT2("b2b-pm: extraction phase measurement with samples %d\n", nInput);
      
      if ((nInput == NSAMPLES) && (poorMansFit(TH1, NSAMPLES, &tH1Ext) == COMMON_STATUS_OK)) {
        // send command: transmit measured phase value
        sendEvtId    = 0x1fff000000000000;                                        // FID, GID
        sendEvtId    = sendEvtId | ((uint64_t)B2BTEST_ECADO_B2B_PREXT << 36);     // EVTNO
        sendParam    = tH1Ext;
        sendDeadline = getSysTime() + COMMON_AHEADT;
        
        common_ebmWriteTM(sendDeadline, sendEvtId, sendParam);
        
      } // if nInput
      else actStatus = B2BTEST_STATUS_PHASEFAILED;
      
      nTransfer++;
      
      break;
    case B2BTEST_ECADO_B2B_PMINJ :
      *pSharedTH1InjHi = (uint32_t)((TH1 >> 32)    & 0xffffffff);
      *pSharedTH1InjLo = (uint32_t)( TH1           & 0xffffffff);
      
      nInput = 0;
      common_ioCtrlSetGate(1, 2);                                      // enable input gate
      while (nInput < NSAMPLES) {                                      // treat 1st TS as junk
        ecaAction = common_wait4ECAEvent(100, &recDeadline, &recParam, &flagIsLate);
        if (ecaAction == B2BTEST_ECADO_TLUINPUT)  {tStamp[nInput] = recDeadline; nInput++;}
        if (ecaAction == B2BTEST_ECADO_TIMEOUT)   break; 
      } // while nInput
      common_ioCtrlSetGate(0, 2);                                      // disable input gate 

      DBPRINT2("b2b-pm: injection phase measurement with samples %d\n", nInput);
      
      if ((nInput == NSAMPLES) && (poorMansFit(TH1, NSAMPLES, &tH1Inj) == COMMON_STATUS_OK)) {
        // send command: transmit measured phase value
        sendEvtId    = 0x1fff000000000000;                                        // FID, GID
        sendEvtId    = sendEvtId | ((uint64_t)B2BTEST_ECADO_B2B_PRINJ << 36);     // EVTNO
        sendParam    = tH1Inj;
        sendDeadline = getSysTime() + COMMON_AHEADT;
        
        common_ebmWriteTM(sendDeadline, sendEvtId, sendParam);
        
      } // if nInput
      else actStatus = B2BTEST_STATUS_PHASEFAILED;
      
      nTransfer++;
      
      break;
    default :
    break;
  } // switch ecaAction

  status = actStatus; /* chk */
  
  return status;
} // doActionOperation


int main(void) {
  uint64_t tActCycle;                           // time of actual UNILAC cycle
  uint32_t status;                              // (error) status
  uint32_t actState;                            // actual FSM state
  uint32_t pubState;                            // value of published state
  uint32_t reqState;                            // requested FSM state
  uint32_t dummy1;                              // dummy parameter
 
  // init local variables
  reqState       = COMMON_STATE_S0;
  actState       = COMMON_STATE_UNKNOWN;
  pubState       = COMMON_STATE_UNKNOWN;
  status         = COMMON_STATUS_OK;
  common_clearDiag();

  init();                                                                     // initialize stuff for lm32
  initSharedMem();                                                            // initialize shared memory
  common_init((uint32_t *)_startshared, B2BPM_FW_VERSION);                    // init common stuff
  
  while (1) {
    common_cmdHandler(&reqState, &dummy1);                                    // check for commands and possibly request state changes
    status = COMMON_STATUS_OK;                                                // reset status for each iteration

    // state machine
    status = common_changeState(&actState, &reqState, status);                // handle requested state changes
    switch(actState) {                                                        // state specific do actions
      case COMMON_STATE_OPREADY :
        status = doActionOperation(&tActCycle, status);
        if (status == COMMON_STATUS_WRBADSYNC)      reqState = COMMON_STATE_ERROR;
        if (status == COMMON_STATUS_ERROR)          reqState = COMMON_STATE_ERROR;
        break;
      default :                                                               // avoid flooding WB bus with unnecessary activity
        status = common_doActionState(&reqState, actState, status);           // other 'do actions' are handled here
        break;
    } // switch

    // update shared memory
    switch (status) {
      case COMMON_STATUS_OK :                                                 // status OK
        sumStatus = sumStatus |  (0x1 << COMMON_STATUS_OK);                   // set OK bit
        break;
      default :                                                               // status not OK
        if ((sumStatus >> COMMON_STATUS_OK) & 0x1) common_incBadStatusCnt();  // changing status from OK to 'not OK': increase 'bad status count'
        sumStatus = sumStatus & ~(0x1 << COMMON_STATUS_OK);                   // clear OK bit
        sumStatus = sumStatus |  (0x1 << status);                             // set status bit and remember other bits set
        break;
    } // switch status
    
    if ((pubState == COMMON_STATE_OPREADY) && (actState  != COMMON_STATE_OPREADY)) common_incBadStateCnt();
    common_publishSumStatus(sumStatus);
    pubState = actState;
    common_publishState(pubState);
    *pSharedNTransfer = nTransfer;
  } // while

  return(1); // this should never happen ...
} // main
