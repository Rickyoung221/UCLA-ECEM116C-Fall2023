# Project Spec:

Objective: The task is to design a memory hierarchy and a memory controller using C++ and standard libraries. The project involves handling memory operations and implementing a cache system.

Specifications:

Programming Language: The project must be done in C++.

## Cache Design: (byte addressable)

- L1 Cache: Direct-mapped with 16 lines, 4 bytes per block.

- L2 Cache: 8-way set-associative with 16 sets, 4 bytes per block, 1 block per line, using LRU replacement policy.

- Victim Cache (btween L1 and L2): Fully-associative with 4 entries, 4 bytes per block, using LRU replacement policy.
- Main Memory: 4096 lines each one 1 byte. No miss in main memory and the address provided in the trace vis the index for the memory array. 

Note that data evicted from LA should be installed in the victim cache (any set from L1 can install their line into the victim cache). The oldest line in the victim cache should be then replaced by this new line and the evicted line from the victim cache should be installed in L2 (and the evicted line from L2 should be removed). 

Memory and Cache Operations: Handle loading (LW) and storing (SW) of data based on a 32-bit address. Implement correct calculations for index, block offset, and tag.

|              | L1 (DM) | L2 (SA) | Victim (FA) |
| ------------ | ------- | ------- | ----------- |
| index        | 4       | 4       | 0           |
| Block Offset | 2       | 2       | 2           |
| Tag          | 26      | 26      | 30          |

Data Handling: Cache design is exclusive/write-no-allocate/write-through, with <u>initial valid bits set to zero.</u>

- write through: always write to **both** cache and main memory

- Exclusive: data is only in one of the levels. 

- Write-no-allocate: (bypass cache and just store in main memory)

## logic

On an L1 miss, first search victim cache:

- If is here, update the victim cache hit stat, bring the data back to L1 (swap)
  - Update the LRU states for the victim (with the new line as the most recent)
- If data is not in, search L2. If data is in L2 or memory, installed in L1. 

Trace Files: The program will read trace files, with each line containing a memory read or write instruction, an address, and data.

Controller Function: The main function will call the memory controller for each trace item, and the controller will manage data between caches and memory, including updating stats and managing data flow.

Performance Metrics: The program should print the **miss-rate for L1 and L2**, and average access time (AAT), with specified cycle times for each memory level.



For lw: search L1, then victim cache, and then L2. If none of them, bring from memory.  ( no miss in main memory)

Code Requirements: Code must be modular, well-commented, and compiled with a specific command. The submission should follow the provided format strictly.

Collaboration Policy: Code sharing is prohibited, but discussions and asking questions are encouraged within the guidelines.

Deliverables:

Submit well-commented C++ source files on Gradescope, ensuring compatibility with the provided skeleton code and trace files.

This assignment requires a comprehensive understanding of memory hierarchy, cache management, and C++ programming. The focus is on implementing a two-level cache system with a victim cache, handling memory operations efficiently, and calculating performance metrics.