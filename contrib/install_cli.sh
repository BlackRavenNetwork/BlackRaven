#!/usr/bin/env bash

# Execute this file to install the BlackRaven CLI tools into your path on OS X

CURRENT_LOC="$( cd "$(dirname "$0")" ; pwd -P )"
LOCATION=${CURRENT_LOC%BlackRaven-Qt.app*}

# Ensure that the directory to symlink to exists
sudo mkdir -p /usr/local/bin

# Create symlinks to the cli tools
sudo ln -sf "${LOCATION}/BlackRaven-Qt.app/Contents/MacOS/blackravend" /usr/local/bin/blackravend
sudo ln -sf "${LOCATION}/BlackRaven-Qt.app/Contents/MacOS/blackraven-cli" /usr/local/bin/blackraven-cli
