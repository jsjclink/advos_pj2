#!/bin/sh

make lib && make && make app
export LD_LIBRARY_PATH=./
./bin/service --n_sms 10 --sms_size 512