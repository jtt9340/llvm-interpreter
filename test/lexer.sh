#!/usr/bin/env bash
# lexer.sh: Rudimentary unit tests for lexer.cpp

# Exit on first error
set -e

# If the name of the resulting binary changes, then this will have to change
# The exe name can also be passed as an argument to this script:
# test/lexer.sh target/release/kaleidescope
exe_name=lexer

# If the output of the above binary every changes, then this will have to change
regex='number \(-[[:digit:]]\)'

if [[ -n $1 ]]; then
  exe=($1)
else
  exe=($(find . -name $exe_name))
fi

case ${#exe[@]} in
  0)
    echo Could not find main executable \"$exe_name\". >&2
    echo Ensure you have compiled the interpreter before running this test script. >&2
    exit 1
    ;;
  1)
    # Flatten the array into a single string
    exe=${exe[0]}
    ;;
  *)
    # Recursively call this script with each executable in the exe array
    for x in ${exe[*]}; do
      # Recursively call this script in parallel
      $0 $x &
    done
    wait # Wait for the recursive calls of this script to finish
    exit $?
    ;;
esac

# Usage: do_test <input> <regex of expected output>
function do_test() {
  local input=$1
  local expr=$2

  echo echo $input \| $exe
  # We have to temporarily disable exit on first error (-e)
  # Because $exe might exit with a non-zero return status which
  # causes this script to silently fail
  set +e
  output=$(echo $input | $exe 2>&1)
  set -e
  if [[ ! $output =~ $expr ]]; then
    echo Unexpected output for input $input >&2
    echo Expected output to match the regular expression \"$expr\" >&2
    echo '	'But instead output was: $output >&2
    exit 1
  fi
}

# Positive path: these cases should work
for input in 1 1.2 23423.3498435793 0.0001 238. .7839; do
  # Run the tests in parallel with &
  do_test "$input" "$regex" &
done

# Negative path: these cases should fail
for input in .; do
  # Run the tests in parallel with &
  do_test "$input" 'invalid token' &
done

wait # Wait for pending unit tests to finish
echo "Passed! ($exe)"
