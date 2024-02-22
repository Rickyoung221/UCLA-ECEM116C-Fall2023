# Project Spec:

Objective: The task is to design a memory hierarchy and a memory controller using C++ and standard libraries. The project involves handling memory operations and implementing a cache system.

Programming Language: The project must be done in C++.

## Cache Design: (byte addressable)

- L1 Cache: Direct-mapped with 16 lines, 4 bytes per block.

- L2 Cache: 8-way set-associative with 16 sets, 4 bytes per block, 1 block per line, using LRU replacement policy.

- Victim Cache (between L1 and L2): Fully-associative with 4 entries, 4 bytes per block, using LRU replacement policy.
- Main Memory: 4096 lines each one 1 byte. No miss in main memory and the address provided in the trace vis the index for the memory array. 

Note that data evicted from LA should be installed in the victim cache (any set from L1 can install their line into the victim cache). The oldest line in the victim cache should be then replaced by this new line and the evicted line from the victim cache should be installed in L2 (and the evicted line from L2 should be removed). 

Memory and Cache Operations: Handle loading (LW) and storing (SW) of data based on a 32-bit address. Implement correct calculations for index, block offset, and tag.

Note：

- **Index Locates the Cache Line:** The index part of the address is used to find the appropriate cache line (or set of lines, in a set-associative cache).
- **Tag for Verification:** Once the correct line is located using the index, the tag stored in the cache line is compared with the tag part of the address to confirm whether the cache line holds the desired data.
- **Block Offset for Data Extraction:** Finally, if there's a cache hit, the block offset is used to extract the specific data from within the cache block.

|              | L1 (DM) | L2 (SA) | Victim Cache(FA) |
| ------------ | ------- | ------- | ---------------- |
| index        | 4       | 4       | 0                |
| Block Offset | 2       | 2       | 2                |
| Tag          | 26      | 26      | 30               |

Data Handling: Cache design is exclusive/write-no-allocate/write-through, with <u>initial valid bits set to zero.</u>

- write through: always write to **both** cache and main memory

- Exclusive: data is only in one of the levels. 

- Write-no-allocate: (bypass cache and just store in main memory)

## logic

### Load 

- First search L1:
  - hit, read the data
  - miss, first search **victim cache**:
    - Hit:
      - update the victim cache hit stat, swap data with the corresponding line (based on the index) in L1 (choose the line with LRU = 0, update it to maxLRU), CPU read the data from L1. 
      - Update the LRU states for the victim (with the new line as the most recent)
        - evict the oldest (LRU=0), then make it 3, LRU position for other entries -1 
    - Miss, search L2:
      - Hit: installed in L1 (not in victim cache)
      - Miss: search in Main memory

- If data is not in, search L2. If data is in L2 or memory, installed in L1. 

Note that if the cache is full (for example in Victim cache, 4 entries are full, evict the one with LRU 0; in L2, if all tags do not match for the index, evict the one with LRU 0) move to the next level. Also, since out design is byte-accessible, we will use block_offset to select which byte in the block. 

Valid bits are used for the very first accesses. Once Victim cache is full, you have to follow LRU.

### Store

Exclusive, write-through, write-no-allocate

- Store to Memory, begin search the cache:
  - On a cache hit, you store the data in both the cache and the memory (due to write-through), update the LRU.
    - L1: hit, nothing new happens, return; miss, search VC
    - VC: hit, **update LRU**, return; miss, search L2
    - L2: hit, **update LRU**, return; miss, return, store the data only in memory (write-no-allocate);  


​	- If the data is already in the cache, update it and overwrite if needed. You shouldn't promote the data to L1 if it is in a higher level (Victim or L2). Leave the data where it is. -If the data is not in the cache, write back to main memory and do not cache it.



Trace Files: The program will read trace files, with each line containing a memory read or write instruction, an address, and data.

Controller Function: The main function will call the memory controller for each trace item, and the controller will manage data between caches and memory, including updating stats and managing data flow.

Performance Metrics: The program should print the **miss-rate for L1 and L2**, and average access time (AAT), with specified cycle times for each memory level.



For lw: search L1, then victim cache, and then L2. If none of them, bring from memory.  ( no miss in main memory)

Code Requirements: Code must be modular, well-commented, and compiled with a specific command. The submission should follow the provided format strictly.

Collaboration Policy: Code sharing is prohibited, but discussions and asking questions are encouraged within the guidelines.

Deliverables:

Submit well-commented C++ source files on Gradescope, ensuring compatibility with the provided skeleton code and trace files.

This assignment requires a comprehensive understanding of memory hierarchy, cache management, and C++ programming. The focus is on implementing a two-level cache system with a victim cache, handling memory operations efficiently, and calculating performance metrics.



## Benchmarks

AAT calculation need to include victim cache:

![Screenshot 2023-11-29 at 21.01.34](https://p.ipic.vip/cvdipy.png)



Correct Output form Campus Wire:

```
mini_debug.txt: (0.25,1.0,28.25)
victim-debug.txt: (0.72727,1.0,60.63636)
L2-test.txt: (0.82353,0.71429,67.23529)
```

