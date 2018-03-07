#!/bin/sh
# starts and configures the firmware (lm32) and software (host) of dm-unipz 

# set -x

###########################################
# clean up stuff
###########################################
echo -e dm-unipz - start: bring possibly resident firmware to idle state
dmunipz-ctl dev/wbm0 stopop
sleep 5

dmunipz-ctl dev/wbm0 idle
sleep 5
echo -e dm-unipz - start: destroy all unowned conditions for lm32 channel of ECA
saft-ecpu-ctl baseboard -x


###########################################
# load firmware to lm32
###########################################
echo -e dm-unipz - start: load firmware
eb-fwload dev/wbm0 u 0x0 dmunipz.bin

###########################################
# start software on hostsystem 
###########################################
# to be implemented: command with pipe to logger (logstash)
echo - dm-unipz - start monitoring
dmunipz-ctl -s2 dev/wbm0 | logger -t dmunipz-ctl -sp local0.info &

###########################################
# configure firmware and make it operational 
###########################################
 
# do some write actions to set register values
echo -e dm-unipz - set MAC and IP of gateway and Data Master
# development: Programmentwicklungsraum, integration network, tsl008, scuxl0033
#dmunipz-ctl dev/wbm0 ebmdm 0x00267b000422 0xc0a80c04
#dmunipz-ctl dev/wbm0 ebmlocal 0x00267b000321 0xc0a80cea
# development: Programmentwicklungsraum, development network tsl015, scuxl0033
dmunipz-ctl dev/wbm0 ebmdm 0x00267b000408 0xc0a88040
dmunipz-ctl dev/wbm0 ebmlocal 0x00267b000321 0xc0a8a00c
# production: BG2, tsl017, scuxl0223
#dmunipz-ctl dev/wbm0 ebmdm 0x00267b000407 0xc0a8803f
#dmunipz-ctl dev/wbm0 ebmlocal 0x00267b0003f1 0xc0a8a0e5

echo -e dm-unipz - start: make firmware operational
# send CONFIGURE command to firmware
sleep 5
dmunipz-ctl dev/wbm0 configure

###########################################
# configure ECA for DM
###########################################
echo -e dm-unipz - configure ECA for events from DM

# configure ECA for lm32 channel: listen for TK request, tag "0x2"
saft-ecpu-ctl tr0 -c 0x1fa215e000000000 0xfffffff000000000 0 0x2 -d

# configure ECA for lm32 channel: listen for beam request, tag "0x3"
saft-ecpu-ctl tr0 -c 0x1fa2160000000000 0xfffffff000000000 0 0x3 -d

# configure ECA for lm32 channel: listen for TK release, tag "0x4"
saft-ecpu-ctl tr0 -c 0x1fa215f000000000 0xfffffff000000000 0 0x4 -d


###########################################
# configure TLU and ECA for UNIPZ
# MIL event EVT_READY_TO_SIS is received as TTL
###########################################
echo -e dm-unipz - configure TLU and ECA for events from UNIPZ

# configure TLU (input B1, TLU will generate messages with event ID
saft-io-ctl tr0 -n B1 -b 0xffff100000000000

# configure ECA for lm32 channel: listen for event ID from TLU, tag "0x6"
saft-ecpu-ctl tr0 -c 0xffff100000000001 0xffffffffffffffff 0 0x6 -d

# send START OPERATATION command to firmware
sleep 5
echo -e dm-unipz - start operation
dmunipz-ctl dev/wbm0 startop

echo -e dm-unipz - start: startup script finished

###########################################
# testing without datamaster
###########################################
# saft-dm bla -fp -n 3600000 schedule.txt
# alternative : 
# - saft-ctl bla -fp inject 0x2222000000000002 0x0 0
# - saft-ctl bla -fp inject 0x3333000000000002 0x0 500000000
# - saft-ctl bla -fp inject 0x4444000000000002 0x0 800000000
# note that action of firmware is triggered by tag and
# that virtual accelerator is specified by low bits of EvtID

