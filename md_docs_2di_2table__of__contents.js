var md_docs_2di_2table__of__contents =
[
    [ "Allocator", "md_docs_2di_2allocator.html", [
      [ "Purpose", "md_docs_2di_2allocator.html#autotoc_md15", null ],
      [ "Problem with Standard C++ Allocators", "md_docs_2di_2allocator.html#autotoc_md16", null ],
      [ "Allocator Interface", "md_docs_2di_2allocator.html#autotoc_md17", null ],
      [ "Allocator Usage", "md_docs_2di_2allocator.html#autotoc_md18", null ],
      [ "Allocator Interaction with Containers", "md_docs_2di_2allocator.html#autotoc_md19", null ],
      [ "Conceptual Example", "md_docs_2di_2allocator.html#autotoc_md20", null ]
    ] ],
    [ "Execution", "md_docs_2di_2execution.html", [
      [ "Purpose", "md_docs_2di_2execution.html#autotoc_md22", null ],
      [ "Conceptual Overview", "md_docs_2di_2execution.html#autotoc_md23", [
        [ "Life Time Model", "md_docs_2di_2execution.html#autotoc_md24", null ]
      ] ],
      [ "Async Sequences", "md_docs_2di_2execution.html#autotoc_md25", [
        [ "Who calls set_next()?", "md_docs_2di_2execution.html#autotoc_md26", null ],
        [ "set_next() Allows the Receiver to Communicate Back to the Sequence", "md_docs_2di_2execution.html#autotoc_md27", null ],
        [ "Async Sequence Life Time Model", "md_docs_2di_2execution.html#autotoc_md28", null ],
        [ "Lockstep Sequences", "md_docs_2di_2execution.html#autotoc_md29", null ],
        [ "How do completion signatures work with sequences?", "md_docs_2di_2execution.html#autotoc_md30", null ],
        [ "Comparison with libunifex Models", "md_docs_2di_2execution.html#autotoc_md31", null ],
        [ "Drawbacks of the Async Sequence Model", "md_docs_2di_2execution.html#autotoc_md32", null ]
      ] ],
      [ "Async RAII", "md_docs_2di_2execution.html#autotoc_md33", [
        [ "The Async Call Stack", "md_docs_2di_2execution.html#autotoc_md34", null ],
        [ "Cleanup can be Asynchronous", "md_docs_2di_2execution.html#autotoc_md35", null ],
        [ "Cleanup can be Fallible", "md_docs_2di_2execution.html#autotoc_md36", null ],
        [ "Async RAII Working Design", "md_docs_2di_2execution.html#autotoc_md37", null ],
        [ "use_resources() Implementation", "md_docs_2di_2execution.html#autotoc_md38", null ],
        [ "make_deferred Implementation", "md_docs_2di_2execution.html#autotoc_md39", null ],
        [ "Async RAII in Coroutines", "md_docs_2di_2execution.html#autotoc_md40", null ]
      ] ],
      [ "Async Scope", "md_docs_2di_2execution.html#autotoc_md41", [
        [ "Nest", "md_docs_2di_2execution.html#autotoc_md42", null ],
        [ "Spawn", "md_docs_2di_2execution.html#autotoc_md43", null ],
        [ "Spawn Future", "md_docs_2di_2execution.html#autotoc_md44", null ],
        [ "Counting Scope", "md_docs_2di_2execution.html#autotoc_md45", null ],
        [ "Benefits of Async Scope Abstraction", "md_docs_2di_2execution.html#autotoc_md46", null ]
      ] ],
      [ "Type Erased Sender", "md_docs_2di_2execution.html#autotoc_md47", [
        [ "How does this work?", "md_docs_2di_2execution.html#autotoc_md48", null ],
        [ "Problems with this Approach", "md_docs_2di_2execution.html#autotoc_md49", [
          [ "Case 1: Creating the Type-Erased Receiver Fails", "md_docs_2di_2execution.html#autotoc_md50", null ],
          [ "Case 2: Creating the Type-Erased Sender Fails", "md_docs_2di_2execution.html#autotoc_md51", null ],
          [ "Case 3: Creating the Type-Erased Operation State Fails", "md_docs_2di_2execution.html#autotoc_md52", null ]
        ] ]
      ] ],
      [ "References", "md_docs_2di_2execution.html#autotoc_md53", null ]
    ] ],
    [ "Intrusive Containers", "md_docs_2di_2intrusive.html", [
      [ "Comparison with Owning Containers", "md_docs_2di_2intrusive.html#autotoc_md55", null ],
      [ "Main Concern with Intrusive Containers", "md_docs_2di_2intrusive.html#autotoc_md56", null ],
      [ "Intrusive Container Customizations", "md_docs_2di_2intrusive.html#autotoc_md57", null ],
      [ "Using the IntrusiveList class", "md_docs_2di_2intrusive.html#autotoc_md58", null ]
    ] ],
    [ "Inter-Process Communication", "md_docs_2di_2ipc.html", [
      [ "Purpose", "md_docs_2di_2ipc.html#autotoc_md60", null ],
      [ "Conceptual Overview", "md_docs_2di_2ipc.html#autotoc_md61", [
        [ "Sending Messages", "md_docs_2di_2ipc.html#autotoc_md62", null ],
        [ "Receiving Messages", "md_docs_2di_2ipc.html#autotoc_md63", null ],
        [ "Sending and Receiving Messages", "md_docs_2di_2ipc.html#autotoc_md64", null ],
        [ "Connection Management", "md_docs_2di_2ipc.html#autotoc_md65", null ]
      ] ],
      [ "Usage", "md_docs_2di_2ipc.html#autotoc_md66", [
        [ "Defining a Message Type", "md_docs_2di_2ipc.html#autotoc_md67", null ],
        [ "Creating a Connection", "md_docs_2di_2ipc.html#autotoc_md68", null ]
      ] ],
      [ "Synchronization", "md_docs_2di_2ipc.html#autotoc_md69", null ]
    ] ],
    [ "Serialization", "md_docs_2di_2serialization.html", [
      [ "Purpose", "md_docs_2di_2serialization.html#autotoc_md71", null ],
      [ "Usage", "md_docs_2di_2serialization.html#autotoc_md72", null ],
      [ "Custom Serialization Formats", "md_docs_2di_2serialization.html#autotoc_md73", null ],
      [ "Custom Deserialization Formats", "md_docs_2di_2serialization.html#autotoc_md74", null ]
    ] ],
    [ "Static Reflection", "md_docs_2di_2static__reflection.html", [
      [ "Purpose", "md_docs_2di_2static__reflection.html#autotoc_md76", null ],
      [ "Note on C++", "md_docs_2di_2static__reflection.html#autotoc_md77", null ],
      [ "Usage", "md_docs_2di_2static__reflection.html#autotoc_md78", null ],
      [ "Internal Representation", "md_docs_2di_2static__reflection.html#autotoc_md79", [
        [ "Atoms", "md_docs_2di_2static__reflection.html#autotoc_md80", null ]
      ] ],
      [ "Accessing Reflection Information", "md_docs_2di_2static__reflection.html#autotoc_md81", null ],
      [ "Uses in library", "md_docs_2di_2static__reflection.html#autotoc_md82", null ],
      [ "Limitations", "md_docs_2di_2static__reflection.html#autotoc_md83", null ]
    ] ],
    [ "Type Erasure", "md_docs_2di_2type__erasure.html", [
      [ "Traditional OOP", "md_docs_2di_2type__erasure.html#autotoc_md86", null ],
      [ "Type Erasure", "md_docs_2di_2type__erasure.html#autotoc_md87", [
        [ "Universality with Concepts", "md_docs_2di_2type__erasure.html#autotoc_md88", null ]
      ] ],
      [ "Ergonomic Concerns", "md_docs_2di_2type__erasure.html#autotoc_md89", [
        [ "Templated Dispatch", "md_docs_2di_2type__erasure.html#autotoc_md90", null ],
        [ "Expressivity for complex CPOs", "md_docs_2di_2type__erasure.html#autotoc_md91", null ],
        [ "Method Resolution", "md_docs_2di_2type__erasure.html#autotoc_md92", null ]
      ] ],
      [ "Multiple types of erased objects", "md_docs_2di_2type__erasure.html#autotoc_md93", null ],
      [ "Implementation", "md_docs_2di_2type__erasure.html#autotoc_md94", [
        [ "Object Management", "md_docs_2di_2type__erasure.html#autotoc_md95", null ],
        [ "Virtual Table Storage", "md_docs_2di_2type__erasure.html#autotoc_md96", null ],
        [ "Meta Object Representation", "md_docs_2di_2type__erasure.html#autotoc_md97", null ],
        [ "Object Categories", "md_docs_2di_2type__erasure.html#autotoc_md98", null ],
        [ "Any Type Summary", "md_docs_2di_2type__erasure.html#autotoc_md99", null ]
      ] ],
      [ "A Practical Example", "md_docs_2di_2type__erasure.html#autotoc_md100", null ]
    ] ]
];