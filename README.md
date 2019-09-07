# os_2
rewriting os

# Current Issues
* Processes don't share kernel memory (each one has its own page tables)
* Processes don't share kernel vm_region information (each has its own cloned version)