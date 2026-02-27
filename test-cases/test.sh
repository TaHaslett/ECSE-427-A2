#!/bin/bash

src_folder="../src"

# ensure that the script is being run in the right directory
cd "$(dirname "$0")" || exit 1

echo "Compiling..."
make -C "${src_folder}"

passed=0
failed=0
total=0

failed_tests=()

for test in *.txt; do
    # skip result files and P_*.txt files
    if [[ "$test" == *_result*.txt ]] || [[ "$test" == P_* ]]; then
        continue
    fi

    base="${test%.txt}"

    # find all matching result files
    result_files=("${base}"_result*.txt)

    if [ ! -f "${result_files[0]}" ]; then
        echo "Missing results for ${test}"
        continue
    fi

    echo "Running: ${test}..."
    total=$((total + 1))

    "./${src_folder}/mysh" < "${test}" > output.tmp

    match_found=false
    matched_file=""

    for expected in "${result_files[@]}"; do
        if diff -u "$expected" output.tmp > diff.tmp; then
            match_found=true
            matched_file="$expected"
            break
        fi
    done

    if [ "$match_found" = true ]; then
        echo "Pass (matched ${matched_file})"
        passed=$((passed + 1))
    else
        echo "Fail (no matching result)"
        echo "--------------------"
        
        # show diff against first result file for reference
        diff -u "${result_files[0]}" output.tmp
        
        failed=$((failed + 1))
        failed_tests+=("$test")
    fi 

    echo "--------------------"

done

echo
echo "--------------------"
echo "Total:    ${total}"
echo "Passed:   ${passed}"
echo "Failed:   ${failed}"
echo "--------------------"
if [ ${#failed_tests[@]} -gt 0 ]; then
    echo "Failed tests:"
    for test in "${failed_tests[@]}"; do
        echo " - $test"
    done
fi

# remove any subfolders that were created
for dir in */ ; do
[ -d "${dir}" ] && rm -rf "${dir}"
done

rm -f output.tmp diff.tmp
