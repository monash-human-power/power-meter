#!/usr/bin/sh
# Updates the version fields of all code files and version definitions.
# Usage:
# ./update_version.sh 1.2.3

# Work out useful folder names
root_folder="$(realpath $(dirname $0s))/../"
code_folder="${root_folder}power-meter-code"
code_folder_name=$(basename $code_folder)

if [ ! -z "$1" ]; then
    # Argument provided, will use.
    echo "New version '$1' specified. Will update versins in all code files."
    # Update docstring
    find "$code_folder" -type 'f' -exec sed -i "s/^\\( *\\* *\\@version \\+\\)[0-9a-zA-Z.\\-]*/\\1${1}/g" {} +

    # Update VERSION define
    sed -i "s/^\\( *\\#define \\+VERSION \\+\\)\"[0-9a-zA-Z.\\-]*\"/\\1\"${1}\"/g" "$code_folder/defines.h"

    # Update the doxygen version
    sed -i "s/^\\(PROJECT_NUMBER \\+= *\\)\"[0-9a-zA-Z.\\-]*\"/\\1\"${1}\"/g" "Doxyfile"
    echo "$all_files"
else
    echo "No new version given. Not doing anything."
fi