#!/usr/bin/sh
# Updates the date fields of modified files.
# Usage:
# ./update_date.sh

# Work out useful folder names
root_folder="$(realpath $(dirname $0s))/../"
code_folder="${root_folder}power-meter-code"
code_folder_name=$(basename $code_folder)

# Get a list of staged and modified files in the code directory.
changed_files=$(git status -s | awk "/^ *(A|M|AM|MM|R) +$code_folder_name/ {if(\$1 !~ /R/) {print \"$root_folder\" \$2;} else {print \"$root_folder\" \$4;}}")
echo "Changed files:"
echo "$changed_files"

# Check that there are actually changed files
if [ ! -z "$changed_files" ]; then
    # Update the date on each changed file.
    subs_date=$(date -I)
        echo "$changed_files" | while read line ; do
        echo "Updating date in '$line'"
        sed -e "s/^\\( *\\* *\\@date \\+\\)[0-9]\\{4\\}-[0-9]\\{2\\}-[0-9]\\{2\\}/\\1${subs_date}/g" -i "$line"
    done
else
    echo "There are no changed code files to update."
fi