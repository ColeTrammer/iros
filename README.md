# os_2
rewriting os

# Current Issues
* When a signal handler interrupts a sys call, the sys call will leak
  memory if it ever called malloc, since the signal discards the entire
  state of the sys call. A possibly solution is to allocate things to the
  stack instead.
* SA_RESTART does not work properly as the call restores directly to sys_entry, 
  but the interrupt stack is not present.