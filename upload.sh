#!/bin/bash
set -e

# Configuration
TARGET=rpi
TARGET_PATH='~/app'

function log() {
    echo 
    echo -e "\033[1;32m$1\033[0m"
}

# Upload
log "Uploading files to $TARGET:$TARGET_PATH"
cd `dirname $0`
rsync --exclude-from=.rsyncignore -avz . $TARGET:$TARGET_PATH

# Execute
log "Executing run.sh on $TARGET"
ssh $TARGET "cd $TARGET_PATH && chmod +x run.sh && ./run.sh"