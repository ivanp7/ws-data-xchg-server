#!/bin/bash

DATA_OUTPUT_LIMIT=32

DIRECTORY=$(cd `dirname $0` && pwd)
stdbuf -oL $DIRECTORY/ws-data-xchg-server -o$DATA_OUTPUT_LIMIT >> $DIRECTORY/server.log

