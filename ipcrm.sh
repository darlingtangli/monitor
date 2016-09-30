#!/bin/bash

SHM=`ipcs -m | grep 0xabcd0605 | awk '{print $2}'`

if [ -n "$SHM" ]
then
    ipcrm -m $SHM
fi
