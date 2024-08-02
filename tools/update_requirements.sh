#!/usr/bin/bash
# Updates requirements.txt with the currently installed python libraries.
# Very short - mainly included for convenience.
#
# Usage:
# ./requirements.sh

root_folder="$(realpath $(dirname $0s))/../"
cd "$root_folder"
source .venv/bin/activate
pip3 freeze > requirements.txt