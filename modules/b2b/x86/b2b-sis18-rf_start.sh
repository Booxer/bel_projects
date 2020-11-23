#!/bin/sh
# startup script for B2B
#
set -x

###########################################
#dev/wbm0 -> tr0 -> PM
#dev/wbm1 -> tr1 -> CBU
###########################################
export TRPM=$(saft-eb-fwd tr0)
export TRCBU=$(saft-eb-fwd tr1)
#export TRPM=dev/wbm0
#export TRCBU=dev/wbm1

echo -e B2B start script for SIS18 rf room

###########################################
# clean up stuff
###########################################
echo -e b2b: bring possibly resident firmware to idle state
b2b-ctl $TRPM stopop
b2b-ctl $TRCBU stopop
sleep 5

b2b-ctl $TRPM idle
b2b-ctl $TRCBU idle
sleep 5

echo -e b2b: destroy all unowned conditions for lm32 channel of ECA
saft-ecpu-ctl tr0 -x
saft-ecpu-ctl tr1 -x

echo -e b2b: disable all events from I/O inputs to ECA
saft-io-ctl tr0 -w
saft-io-ctl tr1 -w
saft-io-ctl tr0 -x
saft-io-ctl tr1 -x

###########################################
# load firmware to lm32
###########################################
echo -e b2b: load firmware 
eb-fwload $TRPM u 0x0 b2bpm.bin
eb-fwload $TRCBU u 0x0 b2bcbu.bin

echo -e b2b: configure firmware
sleep 5
b2b-ctl $TRPM configure
sleep 5
b2b-ctl $TRPM startop
sleep 5
b2b-ctl $TRCBU configure
sleep 5
b2b-ctl $TRCBU startop

echo -e b2b: configure tr0 for phase measurement TLU
###########################################
# configure PM
###########################################
# IO3 configured as TLU input (from 'DDS')
saft-io-ctl tr0 -n IO3 -o 0 -t 1
saft-io-ctl tr0 -n IO3 -b 0xffffa03000000000

# lm32 listens to TLU
saft-ecpu-ctl tr0 -c 0xffffa03000000001 0xffffffffffffffff 0 0xa03 -d

# lm32 listens to CMD_B2B_PMEXT  message from CBU
saft-ecpu-ctl tr0 -c 0x13a0800000000000 0xfffffff000000000 500000 0x800 -dg
saft-ecpu-ctl tr0 -c 0x13a1800000000000 0xfffffff000000000 500000 0x800 -dg

# diag: generate pulse upon CMD_B2B_TRIGGEREXT message from CBU
saft-io-ctl tr0 -n IO2 -o 1 -t 1
saft-io-ctl tr0 -n IO2 -c 0x112c804000000000 0xfffffff000000000 0 0x0 1 -u
saft-io-ctl tr0 -n IO2 -c 0x112c804000000000 0xfffffff000000000 10000000 0x0 0 -u

# testing pulse upon CMD_B2B_DIAGEXT message from CBU
#saft-io-ctl tr0 -n IO1 -o 1 -t 0
#saft-io-ctl tr0 -n IO1 -c 0x13a0807000000000 0xfffffff000000000 0 0x0 1 -u
#saft-io-ctl tr0 -n IO1 -c 0x13a0807000000000 0xfffffff000000000 10000000 0x0 0 -u
#saft-io-ctl tr0 -n IO1 -c 0x13a1807000000000 0xfffffff000000000 0 0x0 1 -u
#saft-io-ctl tr0 -n IO1 -c 0x13a1807000000000 0xfffffff000000000 10000000 0x0 0 -u

echo -e b2b: configure tr1 as cbu
###########################################
# configure CBU
###########################################
# lm32 listens to EVT_KICK_START1  message from DM, 500us pretrigger
saft-ecpu-ctl tr1 -c 0x112c031000000000 0xfffffff000000000 500000 0x031 -dg

# lm32 listens to CMD_B2B_PREXT message from extraction machine
saft-ecpu-ctl tr1 -c 0x13a0802000000000 0xfffffff000000000 500000 0x802 -dg
saft-ecpu-ctl tr1 -c 0x13a1802000000000 0xfffffff000000000 500000 0x802 -dg

# lm32 listens to CMD_B2B_PRINJ message from injection machine
#saft-ecpu-ctl tr1 -c 0x13a1803000000000 0xfffffff000000000 0 0x803 -d

# diag: generate pulse upon EVT_KICK_START event
saft-io-ctl tr1 -n IO1 -o 1 -t 0
saft-io-ctl tr1 -n IO1 -c 0x112c031000000000 0xfffffff000000000 0 0x0 1 -u
saft-io-ctl tr1 -n IO1 -c 0x112c031000000000 0xfffffff000000000 10000000 0x0 0 -u

