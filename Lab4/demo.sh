#!/bin/sh

set -x
# set -e

rmmod -f mydev
insmod mydev.ko

./writer InputYourName &
./reader [Your PC IP] [anyport>8000] /dev/mydev
