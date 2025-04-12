#!/bin/bash
set -e
. ./config.sh

function log() {
    echo 
    echo -e "\033[1;32m$1\033[0m"
}

# Upload
log "Uploading files to $TARGET:$TARGET_PATH"
cd `dirname $0`
rsync --exclude-from=.rsyncignore -avz . $TARGET:$TARGET_PATH
ssh -t $TARGET "sync"

# Execute
# Note: -t option is used to force pseudo-terminal allocation.
# Program would not receive SIGINT signal if it's not attached to a terminal.
log "Executing run.sh on $TARGET"
ssh -t $TARGET "bash $TARGET_PATH/run.sh"