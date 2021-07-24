#!/bin/sh

set -e

print_usage_and_exit() {
   echo "Usage: $0 <test-executable> <test-name> <output-file>"
   exit 2
}

if [ ! $# -eq 3 ];
then
   print_usage_and_exit
fi

exec >|"$3"

test_cases=`$1 -L`
if [ ! "$test_cases" ];
then
   exit 0
fi

echo "$test_cases" | while IFS= read test_case;
do
   printf 'add_test("%s %s" "%s" -t "%s")\n' "$2" "$test_case" "$1" "$test_case" >>"$3"
done
