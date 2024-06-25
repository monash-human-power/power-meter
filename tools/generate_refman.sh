#!/usr/bin/sh
# Generates a doxygen PDF reference manual
# Usage:
# ./generate_refman.sh

# Make sure of the directory we are running from
root_folder="$(realpath $(dirname $0s))/../"
cd "$root_folder"

# Doxygen
doxygen

# Convert latex to pdf
cd ./docs/latex
make

echo "The output file is: $(realpath refman.pdf)"
