#!/bin/sh

print_usage_and_exit() {
    printf "Usage: %s [-gv]\n" "$0"
    unset OPTIND
    return 2
}

enable_graphics=
while getopts :gv opt
do
    case $opt in
        g)   enable_graphics='graphics=1';;
        v)   enable_graphics='graphics=0';;
        ?)   print_usage_and_exit
             return $?
             ;;
    esac
done

unset OPTIND
export OS_2_CMDLINE="$enable_graphics"
echo "OS_2_CMDLINE=\"$OS_2_CMDLINE\""
