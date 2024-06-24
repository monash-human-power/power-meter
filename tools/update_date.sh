#!/usr/bin/sh
# Updates the date fields of modified files. Also optionally modifies the version field if an argument is provided.
root_folder="$(realpath $(dirname $0s))/../"

# Get a list of staged and modified files in the code directory.
changed_files=$(git status -s | awk "/^ *(A |M |AM|MM) +power-meter-code/ {print \"$root_folder\" \$2}")
echo "$changed_files"

# Update the date on each changed file.
subs_date=$(date -I)
echo "$changed_files" | while read line ; do
   echo "Updating date in '$line'"
   sed -e "s/^\\( *\\* *\\@date \\+\\)[0-9]\\{4\\}-[0-9]\\{2\\}-[0-9]\\{2\\}/\\1${subs_date}/g" -i "$line"
done

# Update the version on each changed file if given.
if [ ! -z "$1" ]; then
    # Argument provided, will use.
    echo "New version '$1' specified. Will update versins in all code files."
    find "${root_folder}power-meter-code" -type 'f' -exec sed -i "s/^\\( *\\* *\\@version \\+\\).*\( *\)/\\1${1}\\2/g" {} +
    echo "$all_files"
fi