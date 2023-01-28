# Device Numbers

## Virtual Character Devices (Major 1)

-   1 - /dev/null
-   2 - /dev/zero
-   3 - /dev/full
-   4 - /dev/urandom

## Pseudo Terminal Multiplexer (Major 2)

-   1 - /dev/ptmx
-   2 - /dev/tty

## Master Psuedo Terminals (Major 3)

-   N - /dev/mttyN

## Slave Psuedo Terminals (Major 4)

-   N - /dev/ttyN

## Block Devices (Major 5)

-   0 - /dev/sda
-   N - /dev/sdaN
-   16 - /dev/sdb
-   N - /dev/sdb{N - 16}
-   32 - /dev/sdc
-   N - /dev/sdc{N - 32}
    ...

## Frame Buffer Devices (Major 6)

-   N - /dev/fbN

## Serial Output (Major 8)

-   N - /dev/serialN
