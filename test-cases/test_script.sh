#!/bin/bash

src_folder="../src"

# ensure that the script is being run in the right directory
cd "$(dirname "$0")" || exit 1

echo "Compiling..."
make -C "${src_folder}"

test=$1

if [[ "$test" == T_*_result*.txt ]] || [[ "$test" == P_* ]]; then
    echo "${test} is not a valid test input file."
    exit 0
fi

base="${test%.txt}"

echo "Running: ${test}..."

"./${src_folder}/mysh" < "${test}" > output.tmp
echo "--------------------"
cat output.tmp
echo "--------------------"

# remove any subfolders that were created
for dir in */ ; do
[ -d "${dir}" ] && rm -rf "${dir}"
done

rm -f output.tmp
