#!/bin/sh

print_usage_and_exit() {
    printf "Usage: %s [-g]\n" "$0"
    unset OPTIND
    return 2
}

enable_graphics=
while getopts :g opt
do
    case $opt in
        g)   enable_graphics='graphics=1';;
        ?)   print_usage_and_exit
    esac
done

unset OPTIND
export OS_2_CMDLINE="$enable_graphics"
echo "OS_2_CMDLINE=\"$OS_2_CMDLINE\""
