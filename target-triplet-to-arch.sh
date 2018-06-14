#!/bin/sh
# Matchs strings like i686-elf or i386-os_2 and returns i386
# Else returns the first part of the string before the first dash
# I.e. x86_64-elf returns x86_64
if echo "$1" | grep -Eq 'i[[:digit:]]86-'; then
  echo i386
else
  echo "$1" | grep -Eo '^[[:alnum:]_]*'
fi