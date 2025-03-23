#!/bin/bash
set -e
. ./settings.sh

ssh -t $TARGET "rm -rf $TARGET_PATH/build $TARGET_PATH/src"