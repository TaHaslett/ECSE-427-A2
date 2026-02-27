#!/bin/bash

src_folder="../src"

# ensure that the script is being run in the right directory
cd "$(dirname "$0")" || exit 1 

echo "Compiling..."
make -C "${src_folder}"

passed=0
failed=0
total=0  
for test in *.txt; do
    if [[ "$test" == *_result.txt ]]; then
        continue
    fi

    expected="${test%.txt}_result.txt"

    if [ ! -f "$expected" ]; then
        echo "Missing results for ${test}"
        continue
    fi

    echo "Running: ${test}..."
    total=$((total + 1))

    "./${src_folder}/mysh" < "${test}" > output.tmp

    if diff -u "$expected" output.tmp > diff.tmp; then
        echo "Pass"
        passed=$((passed + 1))
    else
        echo "Fail"
        echo "--------------------"
        cat diff.tmp
        failed=$((failed + 1))
    fi 

    echo "--------------------"

done

echo
echo "--------------------"
echo "Total:    ${total}"
echo "Passed:   ${passed}"
echo "Failed:   ${failed}"
echo "--------------------"

# remove any subfolders that were created 
for dir in */ ; do 
    [ -d "${dir}" ] && rm -rf "${dir}"
done

rm -f output.tmp diff.tmp