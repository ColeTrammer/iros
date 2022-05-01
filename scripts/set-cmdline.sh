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
        g)   enable_graphics='graphics=1;start_args=-g';;
        v)   enable_graphics='graphics=0;start_args=-v';;
        ?)   print_usage_and_exit
             return $?
             ;;
    esac
done

unset OPTIND
export IROS_CMDLINE="$enable_graphics"
echo "IROS_CMDLINE=\"$IROS_CMDLINE\""
