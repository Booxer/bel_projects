#!/bin/sh
# startup script for CBU

# set -x

# lm32 listens to B2B_START message from DM
saft-ecpu-ctl tr0 -c 0x1fff800000000000 0xfffffff000000000 0 0x800 -d

# lm32 listens to B2B_PREXT message from extraction machine
saft-ecpu-ctl tr0 -c 0x1fff803000000000 0xfffffff000000000 0 0x803 -d






