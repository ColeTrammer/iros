## Issues With Doom

1. The dynamic loader hangs on complicated cyclic dependencies.
     (this was fixed by removing libgcc_s.so from libc.so's dependency),
     (and using USE_LD_AS_NEEDED in the toolchain gcc).
2. The keyboard handling logic is incomplete.
3. The system crash if the window is attempted to be resized.
