#!/bin/sh

cd ..
make libc
cd ports

cd figlet
./build.sh
cd ..

cd kilo
./build.sh
cd ..

cd mysh
./build.sh
cd ..
