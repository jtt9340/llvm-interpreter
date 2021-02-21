#!/usr/bin/env bash
# lexer.sh: Rudimentary unit tests for lexer.cpp

# Exit on first error
set -e

# If the name of the resulting binary changes, then this will have to change
exe_name=kaleidescope

# If the output of the above binary every changes, then this will have to change
regex='number \(-[[:digit:]]\)'

exe=$(find . -name $exe_name)
if [[ -z "$exe" ]]; then
	echo Could not find main executable \"$exe_name\". >&2
	echo Ensure you have compiled the interpreter before running this test script. >&2
	exit 1
fi

# Usage: do_test <input> <regex of expected output>
function do_test() {
	local input=$1
	local expr=$2
	
	echo echo $input \| $exe
	# We have to temporarily disable exit on first error (-e)
	# Because $exe might exit with a non-zero return status which
	# causes this scipt to silently fail
	set +e; output=$(echo $input | $exe 2>&1); set -e
	if [[ ! "$output" =~ $expr ]]; then
		echo Unexpected output for input $input >&2
		echo Expected output to match the regular expression \"$expr\" >&2
		echo '	'But instead output was: $output >&2
		exit 1
	fi
}

# Positive path: these cases should work
for input in 1 1.2 23423.3498435793 0.0001 238. .7839; do
	do_test "$input" "$regex"
done

# Negative path: these cases should fail
for input in .; do
	do_test "$input" 'invalid token'
done

echo Passed!
