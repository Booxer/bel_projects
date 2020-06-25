#!/bin/bash
while ../../../ftmx86/sleep_until_modified.sh $1 ; do neato -n -Tpdf -o ${1//.dot/.pdf} $1 ; done
