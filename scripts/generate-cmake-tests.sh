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

if [ "$NATIVE" = "TRUE" ]; then
   test_cases=`$1 -L`
else
   test_cases=`grep -hoE 'TEST\(.+,.+\)' $IROS_GREP_SOURCES | cut -c 6- | rev | cut -c 2- | rev | tr -d ' ' | tr ',' ':'`
fi
if [ ! "$test_cases" ];
then
   exit 0
fi

echo "$test_cases" | while IFS= read test_case;
do
   cmake_test_name=`printf '%s:%s' "$(echo $2 | cut -c 6-)" "$test_case"`
   serialize_tests=""
   if [ "$NATIVE" = "TRUE" ]; then
      printf 'add_test("%s" %s -t "%s")\n' "$cmake_test_name" "$1" "$test_case" >>"$3"
   else
      printf 'add_test("%s" /bin/sh -c "IROS_ARCH=%s IROS_ROOT=%s IROS_KERNEL=%s IROS_INITRD=%s IROS_DISABLE_NETWORKING=1 IROS_REPORT_STATUS=1 IROS_QUIET_KERNEL=1 %s/scripts/run-test.sh %s -t %s")\n' "$cmake_test_name" "$IROS_ARCH" "$IROS_ROOT" "$IROS_KERNEL" "$IROS_INITRD" "$IROS_ROOT" "$2" "$test_case" >> "$3"
      serialize_tests=" RUN_SERIAL TRUE"
   fi
   printf 'set_tests_properties("%s" PROPERTIES TIMEOUT 30%s)\n' "$cmake_test_name" "$serialize_tests" >>"$3"
done
