#!/bin/sh

while [ $((I = ${I:-0} + 1)) -le 10 ];
do 
    echo $I;
done

echo $0