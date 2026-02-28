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
    if [[ "$test" == *_result*.txt ]] || [[ "$test" == P_* ]] ; then
        continue
    fi
    # only run the T_MT* tests
    if [[ "$test" != T_MT* ]]; then
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

    expected_lines=0
    output_lines=0
    for expected in "${result_files[@]}"; do
        # the tests labeled T_MT*.txt behave non deterministically, 
        # so we will just check that they match the result length, rather than the exact output
        expected_lines=$(wc -l < "$expected")
        output_lines=$(wc -l < output.tmp)
        if [ "$expected_lines" = "$output_lines" ]; then
            echo "Pass (matched ${matched_file})"
        passed=$((passed + 1))
            break
        else
            echo "Fail"
            echo "--------------------"
            
            # show line numbers in the output for easier debugging
            echo expected="${expected_lines}"
            echo output="${output_lines}"
            echo "--------------------"

            
            failed=$((failed + 1))
            failed_tests+=("$test")
        fi
    done
    
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
