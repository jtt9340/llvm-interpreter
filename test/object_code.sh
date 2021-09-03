#!/usr/bin/env bash
# object_code.sh: Test that compiling to object code works

# If the name of the resulting binary ever changes, then this will have to change
# The exe name can also be passed as an argument to this script:
# test/object_code.sh target/release/kaleidoscope
exe_name=kaleidoscope

for arg in "$@"; do
  compiler=$(echo "$arg" | cut -d = -f 2)
  if [[ -n $CC ]]; then
    echo Ambiguous values for CC: you want CC to be "$compiler" >&2
    echo but it was previously defined to be "$CC" >&2
    exit 1
  elif [[ $arg == CC=* ]]; then
    CC=$compiler
  fi
done

if [[ -n $1 ]]; then
  exe=("$1")
else
  exe=()
  while IFS='' read -r exe_file; do
    exe+=("$exe_file")
  done < <(find . -type d \( -path '*examples' -o -path '*scratch' -o -path '*.dSYM' \) -prune -false -o -name $exe_name)
fi

case ${#exe[@]} in
  0)
    echo Could not find main executable \"$exe_name\". >&2
    echo Ensure you have compiled the interpreter before running this script. >&2
    exit 1
    ;;
  1)
    # Flatten the array into a single string
    # shellcheck disable=SC2178
    exe=${exe[0]}
    ;;
  *)
    # Recursively call this script with each executable in the exe array
    for x in ${exe[*]}; do
      $0 "$x"
    done
    exit $?
    ;;
esac

exitcode=0

# At this point $exe is not an array since it is flattened above,
# and execution will only reach this point if $exe is not an array.
# shellcheck disable=SC2128
echo 'def avg(x y) (x + y) / 2;' | ASAN_OPTIONS=detect_container_overflow=0 $exe generic

cat >main.c <<_EOF
#include <stdio.h>

double avg(double x, double y);

int main() {
  double x = 3.0, y = 4.0;
  printf("Average of %.1f and %.1f is %.1f\n", x, y, avg(x, y));
}
_EOF

${CC:-clang} main.c session.o -o main

expected="Average of 3.0 and 4.0 is 3.5"
if [[ $(./main) != "$expected" ]]; then
  echo Compiling to object code did not work properly. >&2
  echo Expected output of the following C program: >&2
  cat main.c >&2
  echo to be: "$expected" >&2
  echo but instead was: "$(./main)" >&2
  exitcode=1
fi

rm -f main.c session.o main
exit $exitcode
