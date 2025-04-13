#!/bin/bash
set -e
. ./config.sh

ssh -t $TARGET "rm -rf $TARGET_PATH/*"
echo "Removed all files from $TARGET_PATH"