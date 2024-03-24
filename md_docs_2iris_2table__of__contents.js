var md_docs_2iris_2table__of__contents =
[
    [ "Iris Kernel Overview", "md_docs_2iris_2design_2overview.html", [
      [ "Reason for Writing a New Kernel for Iros", "md_docs_2iris_2design_2overview.html#autotoc_md167", null ],
      [ "Design Priorities of the Iris Kernel", "md_docs_2iris_2design_2overview.html#autotoc_md168", [
        [ "Aysynchronous", "md_docs_2iris_2design_2overview.html#autotoc_md169", null ],
        [ "Minimal", "md_docs_2iris_2design_2overview.html#autotoc_md170", null ],
        [ "Performant", "md_docs_2iris_2design_2overview.html#autotoc_md171", null ]
      ] ],
      [ "Micro-Kernel vs. Monolithic Kernel", "md_docs_2iris_2design_2overview.html#autotoc_md172", [
        [ "Pros of a Micro-Kernel", "md_docs_2iris_2design_2overview.html#autotoc_md173", null ],
        [ "Cons of a Micro-Kernel", "md_docs_2iris_2design_2overview.html#autotoc_md174", null ],
        [ "Pros of a Monolithic Kernel", "md_docs_2iris_2design_2overview.html#autotoc_md175", null ],
        [ "Cons of a Monolithic Kernel", "md_docs_2iris_2design_2overview.html#autotoc_md176", null ],
        [ "General Analysis", "md_docs_2iris_2design_2overview.html#autotoc_md177", null ],
        [ "File System Analysis", "md_docs_2iris_2design_2overview.html#autotoc_md178", null ],
        [ "IPC Wake Up", "md_docs_2iris_2design_2overview.html#autotoc_md179", null ]
      ] ]
    ] ],
    [ "Development Plan", "md_docs_2iris_2design_2development__plan.html", [
      [ "Phase 1: Minimal Kernel", "md_docs_2iris_2design_2development__plan.html#autotoc_md112", [
        [ "Minimal Steps Needed to Run a Program (x86_64)", "md_docs_2iris_2design_2development__plan.html#autotoc_md113", null ],
        [ "Next Steps for Running a Simple Program", "md_docs_2iris_2design_2development__plan.html#autotoc_md114", null ],
        [ "Steps Completeable in Isolation", "md_docs_2iris_2design_2development__plan.html#autotoc_md115", null ],
        [ "Note on Microkernel Architecture", "md_docs_2iris_2design_2development__plan.html#autotoc_md116", null ],
        [ "Overall Development Intent", "md_docs_2iris_2design_2development__plan.html#autotoc_md117", null ]
      ] ],
      [ "Phase 2: Full Vertical Slices", "md_docs_2iris_2design_2development__plan.html#autotoc_md118", [
        [ "Vertical Slices", "md_docs_2iris_2design_2development__plan.html#autotoc_md119", null ],
        [ "Horizontal Slices", "md_docs_2iris_2design_2development__plan.html#autotoc_md120", null ],
        [ "File System", "md_docs_2iris_2design_2development__plan.html#autotoc_md121", null ],
        [ "Memory Management", "md_docs_2iris_2design_2development__plan.html#autotoc_md122", null ],
        [ "Process Management", "md_docs_2iris_2design_2development__plan.html#autotoc_md123", null ],
        [ "Time Keeping", "md_docs_2iris_2design_2development__plan.html#autotoc_md124", null ],
        [ "Dynamic Linking", "md_docs_2iris_2design_2development__plan.html#autotoc_md125", null ],
        [ "Networking", "md_docs_2iris_2design_2development__plan.html#autotoc_md126", null ],
        [ "Graphics", "md_docs_2iris_2design_2development__plan.html#autotoc_md127", null ],
        [ "Audio", "md_docs_2iris_2design_2development__plan.html#autotoc_md128", null ],
        [ "Syncronization Primitives", "md_docs_2iris_2design_2development__plan.html#autotoc_md129", null ],
        [ "Containerization", "md_docs_2iris_2design_2development__plan.html#autotoc_md130", null ]
      ] ]
    ] ],
    [ "Driver Interface", "md_docs_2iris_2design_2driver__interface.html", null ],
    [ "Initrd", "md_docs_2iris_2design_2initrd.html", [
      [ "Explanation", "md_docs_2iris_2design_2initrd.html#autotoc_md133", null ],
      [ "Format", "md_docs_2iris_2design_2initrd.html#autotoc_md134", null ],
      [ "Design Diagram", "md_docs_2iris_2design_2initrd.html#autotoc_md135", null ]
    ] ],
    [ "Interrupts", "md_docs_2iris_2design_2interrupts.html", [
      [ "Explanation", "md_docs_2iris_2design_2interrupts.html#autotoc_md137", null ],
      [ "Interrupt Sources", "md_docs_2iris_2design_2interrupts.html#autotoc_md138", null ],
      [ "Hardware Interrupts", "md_docs_2iris_2design_2interrupts.html#autotoc_md139", [
        [ "NMI", "md_docs_2iris_2design_2interrupts.html#autotoc_md140", null ],
        [ "Note for Micro-Kernels", "md_docs_2iris_2design_2interrupts.html#autotoc_md141", null ]
      ] ]
    ] ],
    [ "Memory Allocation", "md_docs_2iris_2design_2memory__management.html", [
      [ "Physical Page Frame Allocator", "md_docs_2iris_2design_2memory__management.html#autotoc_md143", [
        [ "Free-List Allocator", "md_docs_2iris_2design_2memory__management.html#autotoc_md144", null ],
        [ "Bitmap Allocator", "md_docs_2iris_2design_2memory__management.html#autotoc_md145", null ],
        [ "Buddy Allocator", "md_docs_2iris_2design_2memory__management.html#autotoc_md146", null ],
        [ "In the Iris Kernel", "md_docs_2iris_2design_2memory__management.html#autotoc_md147", null ],
        [ "Note on Boot Strapping", "md_docs_2iris_2design_2memory__management.html#autotoc_md148", null ],
        [ "Note on Physical Memory Management Initialization", "md_docs_2iris_2design_2memory__management.html#autotoc_md149", null ],
        [ "Note on NUMA (Non-Uniform Memory Access)", "md_docs_2iris_2design_2memory__management.html#autotoc_md150", null ]
      ] ],
      [ "Virtual Memory Allocator (Page Granularity)", "md_docs_2iris_2design_2memory__management.html#autotoc_md151", null ],
      [ "Heap Allocation (Byte Granular)", "md_docs_2iris_2design_2memory__management.html#autotoc_md152", null ],
      [ "Memory Management", "md_docs_2iris_2design_2memory__management.html#autotoc_md153", [
        [ "Physical Memory Tracking", "md_docs_2iris_2design_2memory__management.html#autotoc_md154", [
          [ "Page Structure Boostrap", "md_docs_2iris_2design_2memory__management.html#autotoc_md155", null ]
        ] ],
        [ "Solving the Boostrapping Problem", "md_docs_2iris_2design_2memory__management.html#autotoc_md156", [
          [ "Allocating the Physical Page Structures", "md_docs_2iris_2design_2memory__management.html#autotoc_md157", null ]
        ] ],
        [ "Memory Region Backing Objects", "md_docs_2iris_2design_2memory__management.html#autotoc_md158", [
          [ "Implementing Shared Memory", "md_docs_2iris_2design_2memory__management.html#autotoc_md159", null ],
          [ "Implementing Memory-Mapped Files", "md_docs_2iris_2design_2memory__management.html#autotoc_md160", null ],
          [ "Implementing COW Memory and the Shared Zero-Page", "md_docs_2iris_2design_2memory__management.html#autotoc_md161", null ],
          [ "Implementing Backing Objects", "md_docs_2iris_2design_2memory__management.html#autotoc_md162", [
            [ "Potential Limitations", "md_docs_2iris_2design_2memory__management.html#autotoc_md163", null ]
          ] ]
        ] ],
        [ "TLB Management", "md_docs_2iris_2design_2memory__management.html#autotoc_md164", [
          [ "TLB Shootdown", "md_docs_2iris_2design_2memory__management.html#autotoc_md165", null ]
        ] ]
      ] ]
    ] ],
    [ "Scheduler", "md_docs_2iris_2design_2scheduler.html", null ],
    [ "Security", "md_docs_2iris_2design_2security.html", null ],
    [ "Synchronization Primitives", "md_docs_2iris_2design_2synchronization.html", [
      [ "Purpose", "md_docs_2iris_2design_2synchronization.html#autotoc_md183", null ],
      [ "Safety", "md_docs_2iris_2design_2synchronization.html#autotoc_md184", [
        [ "Note on Asynchronous Execution", "md_docs_2iris_2design_2synchronization.html#autotoc_md185", null ]
      ] ],
      [ "Locking Mechanisms", "md_docs_2iris_2design_2synchronization.html#autotoc_md186", [
        [ "Spinlock", "md_docs_2iris_2design_2synchronization.html#autotoc_md187", null ],
        [ "Mutex", "md_docs_2iris_2design_2synchronization.html#autotoc_md188", null ],
        [ "Note on Asynchronous Execution", "md_docs_2iris_2design_2synchronization.html#autotoc_md189", null ]
      ] ]
    ] ],
    [ "System Call Interface", "md_docs_2iris_2design_2system__call__interface.html", [
      [ "Traditional Model for System Calls", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md191", null ],
      [ "Asynchronous Model for System Calls", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md192", null ],
      [ "Asynchronous First System Calls", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md193", [
        [ "ABI", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md194", null ],
        [ "Note on Null-Terminated Strings", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md195", null ],
        [ "Emulating Readyness APIs", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md196", null ],
        [ "Emulating Traditional Non-blocking IO", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md197", null ]
      ] ],
      [ "Task Creation", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md198", [
        [ "Emulating POSIX", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md199", null ]
      ] ],
      [ "Platform Specific ABI", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md200", [
        [ "x86_64", "md_docs_2iris_2design_2system__call__interface.html#autotoc_md201", null ]
      ] ]
    ] ],
    [ "Task Model", "md_docs_2iris_2design_2task.html", [
      [ "What is a task?", "md_docs_2iris_2design_2task.html#autotoc_md203", null ],
      [ "How is a process represented?", "md_docs_2iris_2design_2task.html#autotoc_md204", null ],
      [ "How is a task represented?", "md_docs_2iris_2design_2task.html#autotoc_md205", null ],
      [ "Program Initialization ABI", "md_docs_2iris_2design_2task.html#autotoc_md206", null ]
    ] ],
    [ "Virtual File System", "md_docs_2iris_2design_2virtual__file__system.html", [
      [ "What is it?", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md208", null ],
      [ "Orthogonal Concerns", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md209", null ],
      [ "Userspace API", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md210", [
        [ "Paths", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md211", null ],
        [ "File Descriptors", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md212", null ],
        [ "Kernel File System Model", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md213", [
          [ "File System Composition (Mount)", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md214", null ],
          [ "File System Instance (Super Block)", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md215", null ],
          [ "File System Entity (Inode)", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md216", null ],
          [ "File System Anchor (Tnode / dentry)", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md217", null ]
        ] ],
        [ "Path Resolution", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md218", [
          [ "Algorithm", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md219", null ],
          [ "Optimization", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md220", null ]
        ] ],
        [ "Page Cache", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md221", [
          [ "From the Perspective of a File System Driver", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md222", null ],
          [ "From the Perspective of the VFS", "md_docs_2iris_2design_2virtual__file__system.html#autotoc_md223", null ]
        ] ]
      ] ]
    ] ]
];