#!/bin/bash
set -e
cd `dirname $0`

function log() {
    echo 
    echo -e "\033[1;32m$1\033[0m"
}

log "Build app"
mkdir -p build
cd build
cmake ..
make

log "Start app"
sudo ./app

log "Done"