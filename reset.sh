#!/bin/bash
set -e
. ./config.sh

ssh -t $TARGET "rm -rf $TARGET_PATH/build $TARGET_PATH/src"