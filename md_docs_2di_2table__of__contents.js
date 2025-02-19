var md_docs_2di_2table__of__contents =
[
    [ "Allocator", "md_docs_2di_2allocator.html", [
      [ "Purpose", "md_docs_2di_2allocator.html#autotoc_md18", null ],
      [ "Problem with Standard C++ Allocators", "md_docs_2di_2allocator.html#autotoc_md19", null ],
      [ "Allocator Interface", "md_docs_2di_2allocator.html#autotoc_md20", null ],
      [ "Allocator Usage", "md_docs_2di_2allocator.html#autotoc_md21", null ],
      [ "Allocator Interaction with Containers", "md_docs_2di_2allocator.html#autotoc_md22", null ],
      [ "Conceptual Example", "md_docs_2di_2allocator.html#autotoc_md23", null ]
    ] ],
    [ "Bytes", "md_docs_2di_2bytes.html", [
      [ "Purpose", "md_docs_2di_2bytes.html#autotoc_md25", null ],
      [ "Byte Buffer", "md_docs_2di_2bytes.html#autotoc_md26", [
        [ "Type Erased Backing Store", "md_docs_2di_2bytes.html#autotoc_md27", null ],
        [ "Shared Ownership", "md_docs_2di_2bytes.html#autotoc_md28", null ],
        [ "Unique Ownership", "md_docs_2di_2bytes.html#autotoc_md29", null ]
      ] ]
    ] ],
    [ "Execution", "md_docs_2di_2execution.html", [
      [ "Purpose", "md_docs_2di_2execution.html#autotoc_md31", null ],
      [ "Conceptual Overview", "md_docs_2di_2execution.html#autotoc_md32", [
        [ "Life Time Model", "md_docs_2di_2execution.html#autotoc_md33", null ]
      ] ],
      [ "Async Sequences", "md_docs_2di_2execution.html#autotoc_md34", [
        [ "Who calls set_next()?", "md_docs_2di_2execution.html#autotoc_md35", null ],
        [ "set_next() Allows the Receiver to Communicate Back to the Sequence", "md_docs_2di_2execution.html#autotoc_md36", null ],
        [ "Async Sequence Life Time Model", "md_docs_2di_2execution.html#autotoc_md37", null ],
        [ "Lockstep Sequences", "md_docs_2di_2execution.html#autotoc_md38", null ],
        [ "How do completion signatures work with sequences?", "md_docs_2di_2execution.html#autotoc_md39", null ],
        [ "Comparison with libunifex Models", "md_docs_2di_2execution.html#autotoc_md40", null ],
        [ "Drawbacks of the Async Sequence Model", "md_docs_2di_2execution.html#autotoc_md41", null ]
      ] ],
      [ "Async RAII", "md_docs_2di_2execution.html#autotoc_md42", [
        [ "The Async Call Stack", "md_docs_2di_2execution.html#autotoc_md43", null ],
        [ "Cleanup can be Asynchronous", "md_docs_2di_2execution.html#autotoc_md44", null ],
        [ "Cleanup can be Fallible", "md_docs_2di_2execution.html#autotoc_md45", null ],
        [ "Async RAII Working Design", "md_docs_2di_2execution.html#autotoc_md46", null ],
        [ "use_resources() Implementation", "md_docs_2di_2execution.html#autotoc_md47", null ],
        [ "make_deferred Implementation", "md_docs_2di_2execution.html#autotoc_md48", null ],
        [ "Async RAII in Coroutines", "md_docs_2di_2execution.html#autotoc_md49", null ]
      ] ],
      [ "Async Scope", "md_docs_2di_2execution.html#autotoc_md50", [
        [ "Nest", "md_docs_2di_2execution.html#autotoc_md51", null ],
        [ "Spawn", "md_docs_2di_2execution.html#autotoc_md52", null ],
        [ "Spawn Future", "md_docs_2di_2execution.html#autotoc_md53", null ],
        [ "Counting Scope", "md_docs_2di_2execution.html#autotoc_md54", null ],
        [ "Benefits of Async Scope Abstraction", "md_docs_2di_2execution.html#autotoc_md55", null ]
      ] ],
      [ "Type Erased Sender", "md_docs_2di_2execution.html#autotoc_md56", [
        [ "How does this work?", "md_docs_2di_2execution.html#autotoc_md57", null ],
        [ "Problems with this Approach", "md_docs_2di_2execution.html#autotoc_md58", [
          [ "Case 1: Creating the Type-Erased Receiver Fails", "md_docs_2di_2execution.html#autotoc_md59", null ],
          [ "Case 2: Creating the Type-Erased Sender Fails", "md_docs_2di_2execution.html#autotoc_md60", null ],
          [ "Case 3: Creating the Type-Erased Operation State Fails", "md_docs_2di_2execution.html#autotoc_md61", null ]
        ] ]
      ] ],
      [ "References", "md_docs_2di_2execution.html#autotoc_md62", null ]
    ] ],
    [ "Intrusive Containers", "md_docs_2di_2intrusive.html", [
      [ "Comparison with Owning Containers", "md_docs_2di_2intrusive.html#autotoc_md64", null ],
      [ "Main Concern with Intrusive Containers", "md_docs_2di_2intrusive.html#autotoc_md65", null ],
      [ "Intrusive Container Customizations", "md_docs_2di_2intrusive.html#autotoc_md66", null ],
      [ "Using the IntrusiveList class", "md_docs_2di_2intrusive.html#autotoc_md67", null ]
    ] ],
    [ "Inter-Process Communication", "md_docs_2di_2ipc.html", [
      [ "Purpose", "md_docs_2di_2ipc.html#autotoc_md69", null ],
      [ "Conceptual Overview", "md_docs_2di_2ipc.html#autotoc_md70", [
        [ "Sending Messages", "md_docs_2di_2ipc.html#autotoc_md71", null ],
        [ "Receiving Messages", "md_docs_2di_2ipc.html#autotoc_md72", null ],
        [ "Sending and Receiving Messages", "md_docs_2di_2ipc.html#autotoc_md73", null ],
        [ "Connection Management", "md_docs_2di_2ipc.html#autotoc_md74", null ]
      ] ],
      [ "Usage", "md_docs_2di_2ipc.html#autotoc_md75", [
        [ "Defining a Message Type", "md_docs_2di_2ipc.html#autotoc_md76", null ],
        [ "Creating a Connection", "md_docs_2di_2ipc.html#autotoc_md77", null ]
      ] ],
      [ "Synchronization", "md_docs_2di_2ipc.html#autotoc_md78", null ]
    ] ],
    [ "Serialization", "md_docs_2di_2serialization.html", [
      [ "Purpose", "md_docs_2di_2serialization.html#autotoc_md80", null ],
      [ "Usage", "md_docs_2di_2serialization.html#autotoc_md81", null ],
      [ "Custom Serialization Formats", "md_docs_2di_2serialization.html#autotoc_md82", null ],
      [ "Custom Deserialization Formats", "md_docs_2di_2serialization.html#autotoc_md83", null ]
    ] ],
    [ "Static Reflection", "md_docs_2di_2static__reflection.html", [
      [ "Purpose", "md_docs_2di_2static__reflection.html#autotoc_md85", null ],
      [ "Note on C++", "md_docs_2di_2static__reflection.html#autotoc_md86", null ],
      [ "Usage", "md_docs_2di_2static__reflection.html#autotoc_md87", null ],
      [ "Internal Representation", "md_docs_2di_2static__reflection.html#autotoc_md88", [
        [ "Atoms", "md_docs_2di_2static__reflection.html#autotoc_md89", null ]
      ] ],
      [ "Accessing Reflection Information", "md_docs_2di_2static__reflection.html#autotoc_md90", null ],
      [ "Uses in library", "md_docs_2di_2static__reflection.html#autotoc_md91", null ],
      [ "Limitations", "md_docs_2di_2static__reflection.html#autotoc_md92", null ]
    ] ],
    [ "Type Erasure", "md_docs_2di_2type__erasure.html", [
      [ "Traditional OOP", "md_docs_2di_2type__erasure.html#autotoc_md95", null ],
      [ "Using Type Erasure", "md_docs_2di_2type__erasure.html#autotoc_md96", [
        [ "Universality with Concepts", "md_docs_2di_2type__erasure.html#autotoc_md97", null ]
      ] ],
      [ "Ergonomic Concerns", "md_docs_2di_2type__erasure.html#autotoc_md98", [
        [ "Templated Dispatch", "md_docs_2di_2type__erasure.html#autotoc_md99", null ],
        [ "Expressivity for complex CPOs", "md_docs_2di_2type__erasure.html#autotoc_md100", null ],
        [ "Method Resolution", "md_docs_2di_2type__erasure.html#autotoc_md101", null ]
      ] ],
      [ "Multiple types of erased objects", "md_docs_2di_2type__erasure.html#autotoc_md102", null ],
      [ "Implementation", "md_docs_2di_2type__erasure.html#autotoc_md103", [
        [ "Object Management", "md_docs_2di_2type__erasure.html#autotoc_md104", null ],
        [ "Virtual Table Storage", "md_docs_2di_2type__erasure.html#autotoc_md105", null ],
        [ "Meta Object Representation", "md_docs_2di_2type__erasure.html#autotoc_md106", null ],
        [ "Object Categories", "md_docs_2di_2type__erasure.html#autotoc_md107", null ],
        [ "Any Type Summary", "md_docs_2di_2type__erasure.html#autotoc_md108", null ]
      ] ],
      [ "A Practical Example", "md_docs_2di_2type__erasure.html#autotoc_md109", null ]
    ] ]
];