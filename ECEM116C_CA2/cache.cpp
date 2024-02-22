#include "cache.h"

cache::cache() // initiate
{
  // initialize L1, L2, Victim cache (4 entries)
  for (int i = 0; i < L1_CACHE_SETS; i++)
  {
    L1[i].valid = false;
    L1[i].data = 0;
    L1[i].tag = -1; // An invalid tag
  }
  for (int i = 0; i < L2_CACHE_SETS; i++)
    for (int j = 0; j < L2_CACHE_WAYS; j++)
    {
      L2[i][j].valid = false;
      L2[i][j].data = 0;
      L2[i][j].lru_position = j;
      L2[i][j].tag = -1; // An invalid tag
    }
  for (int i = 0; i < VICTIM_SIZE; i++)
  {
    VC[i].valid = false;
    VC[i].data = 0;
    VC[i].lru_position = i; // initial LRU position
    VC[i].tag = -1;         // An invalid tag
  }
  // Do the same for Victim Cache ...
  this->myStat.missL1 = 0;
  this->myStat.missL2 = 0;
  this->myStat.hitL1 = 0;
  this->myStat.hitL2 = 0;
  this->myStat.accL1 = 0;
  this->myStat.accL2 = 0;
  // Add stat for Victim cache ...
  this->myStat.missVic = 0;
  this->myStat.hitVic = 0;
  this->myStat.accVic = 0;
}

void cache::controller(bool MemR, bool MemW, int *data, int adr, int *myMem)
{
  // Extract block offset, index, tag for L1 and L2, tag for victim cache
  int block_offset = adr & 0x3;                      // Last 2 bits for block offset
  int index = (adr >> BLOCK_OFFSET_BITS) & 0xF;      // For L1 and L2, victim does not have index (FA)
  int tag = adr >> (BLOCK_OFFSET_BITS + INDEX_SIZE); // for L1 and L2,the remaining 26 bits
  int tag_victim = adr >> (BLOCK_OFFSET_BITS);       // tag for victim cache, 30 bits
  if (MemR)
  { // Load instruction
    *data = loadWord(block_offset, tag, index, tag_victim, adr, myMem);
  }
  if (MemW)
  { 
    myMem[adr] = *data; // Write to memory
    storeWord(block_offset, tag, index, adr, tag_victim, *data);
  }
}

/* LOAD
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
*/

int cache::loadWord(int block_offset, int tag, int index, int tag_victim, int address, int *myMem)
{
  int data;
  bool hit = false;
  // Check L1 Cache
  // clog << "search beign" << endl;
  hit = searchL1(index, tag, data);
  myStat.accL1++; // accesses to L1 cache
  clog << "+++++++++++++Begin++++++++++++++" << endl;
  clog << "index: " << index << endl;
  clog << "tag: " << tag << endl;
  clog << "address: " << address << endl;
  // clog << "tag for victim: " << tag_victim << endl;
  clog << "+++++++++++++++++++++++++++++++" << endl;
  clog << "accL1:" << myStat.accL1 << endl;

  if (hit)
  {
    // myStat.hitL1++;
    clog << "l1 hit!" << endl;
    return data; // tag match
  }
  else
  {
    myStat.missL1++; // h1, miss
    myStat.accVic++; // check vc
    clog << "L1 ---> VC" << endl;
    clog << "accVic:" << myStat.accVic << endl;
    clog << "miss time for L1:" << myStat.missL1 << endl;
    // clog << "tag for VC: " << tag_victim << endl;
    hit = searchVictimCache(tag_victim, data); // update lru during searching
    if (hit)
    {
      // myStat.hitVic++;
      //clog << "vc hit, swap to l1" << endl;
      swapL1Victim(address); // hit in victim cache, Data is swapped with the corresponding line in L1
      return L1[index].data;
    }
    else
    {
      myStat.missVic++; // victim miss
      myStat.accL2++;
      clog << "L1 ---> VC ---> L2" << endl;
      //clog << "tag for victim: " << tag_victim << endl;
      //clog << "accL2:" << myStat.accL2 << endl;
      //clog << "miss time for VC:" << myStat.missVic << endl;
      // Check L2 Cache
      hit = searchL2(index, tag, data);
      if (hit)
      {
        //  Data is moved from L2 to L1,
        //  evcit the corresponding line (same index)
        //  update L1 and evict the line if there is conflict
        cacheBlock evictedL1 = updateL1(index, tag, data, block_offset, tag_victim, address);
        if (evictedL1.valid)
        {
          // Handle eviction from L1 to Victim cache, update lru in vc as well
          clog << "L2 hit, move to L1, and evict block from l1 should update vc" << endl;
          cacheBlock evictedVictim = updateVictimCache(evictedL1);
          if (evictedVictim.valid)
          {
            // Handle eviction from Victim cache to L2
            updateL2(evictedVictim);
          }
        } // if the evicted block not valid, no need to move to the next cache level.
        return data;
      }
      else
      {
        myStat.missL2++;
        clog << "L1 ---> VC ---> L2 ---> Memory" << endl;
        //  Load data from Main Memory
        data = loadFromMemory(address, myMem);
        // Update L1 with data from Memory
        cacheBlock evictedL1 = updateL1(index, tag, data, block_offset, tag_victim, address);
        // clog << "tag for victim: " << tag_victim << endl;
        if (evictedL1.valid)
        {
          // Handle eviction from L1 to Victim cache
          clog << "should update vc" << endl;
          cacheBlock evictedVictim = updateVictimCache(evictedL1);
          if (evictedVictim.valid)
          {
            // Handle eviction from Victim cache to L2
            updateL2(evictedVictim);
          }
        }
        return data;
      }
    }
  }
}
int cache::loadFromMemory(int address, int *myMem)
{
  // Ensure the address is within the bounds of the memory array
  if (address < 0 || address >= MEM_SIZE)
  {
    cerr << "Memory access error: Address " << address << " is out of bounds." << endl;
    exit(EXIT_FAILURE); // or handle the error as appropriate
  }
  // Return the data from the specified memory address
  return myMem[address];
}
/* STORE
Exclusive, write-through, write-no-allocate
- Store to Memory (no matter in which case), begin search the cache:
  - On a cache hit, you store the data in both the cache and the memory (due to write-through), update the LRU.
    - L1: hit, nothing new happens, return; miss, search VC
    - VC: hit, **update LRU**, return; miss, search L2
    - L2: hit, **update LRU**, return; miss, return, store the data only in memory (write-no-allocate);  

​	- If the data is already in the cache, update it and overwrite if needed. You shouldn't promote the data to L1 if it is in a higher level (Victim or L2). Leave the data where it is. -If the data is not in the cache, write back to main memory and do not cache it.

*/
void cache::storeWord(int block_offset, int tag, int index, int address, int tag_victim, int data)
{
  // Update L1 and Memory if hit in L1
  if (searchL1(index, tag, data))
  {
    updateL1(index, tag, data, block_offset, tag_victim, address);
    return;
  }
  else // L1 miss
  { 
    if (searchVictimCache(tag_victim, data))
    {
      // Update Victim Cache LRU, hit
      int lruIndex = findVictimCacheReplacementEntry();
      updateLRUVictim(lruIndex);
      return;
    }
    else // Victim Cache miss
    { 
      if (searchL2(index, tag, data)) 
      {
        // Update L2 Cache LRU if hit
        updateLRUL2(index, findL2ReplacementIndex(index));
        return;
      }
      else
      {
        return;
        // Handle L2 miss here if needed
      }
    }
  } 
} 



/*
Search functions in each cache
*/

bool cache::searchL1(int index, int tag, int &data)
{
  if (L1[index].valid && L1[index].tag == tag)
  {
    data = L1[index].data;
    return true; // Hit
  }
  return false; // Miss
}

bool cache::searchVictimCache(int tag_victim, int &data)
{
  // iterate for all the entires
  for (int i = 0; i < VICTIM_SIZE; i++)
  {
    if (VC[i].valid && VC[i].tag_victim == tag_victim)
    {
      data = VC[i].data;
      // Hit, Update LRU for Victim Cache
      updateLRUVictim(i);
      return true; // Hit
    }
  }
  return false; // Miss
}

bool cache::searchL2(int index, int tag, int &data)
{
  for (int way = 0; way < L2_CACHE_WAYS; way++)
  {
    if (L2[index][way].valid && L2[index][way].tag == tag)
    {
      data = L2[index][way].data;
      // Hit, Update LRU for L2
      updateLRUL2(index, way);
      return true; // Hit
    }
  }
  return false; // Miss
}

/*
Update functions in each cache
*/

cacheBlock cache::updateL1(int index, int tag, int newData, int block_offset, int tag_victim, int address)
{
  cacheBlock evictedBlock;
  //  Check if the block at the index is already valid (indicating it needs to be evicted)
  if (L1[index].valid)
  {
    // Save the current block for eviction
    evictedBlock = L1[index];
  } else {
    // If no block is being evicted, set valid to false to indicate this
    evictedBlock.valid = false;
  }
  // Update the L1 cache with the new data
  L1[index].tag = tag;
  L1[index].data = newData;
  L1[index].valid = true;
  L1[index].address = address;
  L1[index].block_offset = block_offset;
  L1[index].tag_victim = tag_victim;
  L1[index].index = index;
  return evictedBlock;
}

cacheBlock cache::updateVictimCache(cacheBlock newBlock) {
    int replaceIndex = findVictimCacheReplacementEntry();
    cacheBlock evictedBlock;
    clog << "Updating Victim Cache." << endl;

    if (replaceIndex >= 0 && VC[replaceIndex].valid == false) {
        // There's an empty slot or a slot to be replaced
        evictedBlock = VC[replaceIndex];
        VC[replaceIndex] = newBlock;
        VC[replaceIndex].valid = true;
        // Set to max LRU, the most recently used
        VC[replaceIndex].lru_position = VICTIM_SIZE - 1; 
        updateLRUVictim(replaceIndex); // Update LRU positions
    } else {
        // If the cache is full, find the block with LRU = 0 to replace
        clog << "Victim Cache is full!" << endl;
        for (int i = 0; i < VICTIM_SIZE; i++) {
            if (VC[i].lru_position == 0) {
                evictedBlock = VC[i];
                VC[i] = newBlock;
                VC[i].valid = true;
                updateLRUVictim(i); // Update LRU positions
                break;
            }
        }
    }
    return evictedBlock; // Return the block that was evicted
}

int cache::findVictimCacheReplacementEntry()
{
  // valid bit for the first accesses
  for (int i = 0; i < VICTIM_SIZE; i++)
  {
    if (!VC[i].valid)
    {
      return i; // Found an empty slot
    }
  }
  // If all slots are valid (full), find the LRU block to replace
  int lruIndex = 0;
  for (int i = 1; i < VICTIM_SIZE; i++)
  {
    if (VC[i].lru_position == 0)
    {
      lruIndex = i;
    }
  }
  return lruIndex; // Return the LRU block's index
}

void cache::updateLRUVictim(int accessedIndex) {
    int maxLRU = VICTIM_SIZE - 1; // 4-1=3 ---- 0,1,2,3
    clog << "LRU in Victim Cache updating!" << endl;

    for (int i = 0; i < VICTIM_SIZE; i++) {
        clog << "Current index: " << i << ", its LRU: " << VC[i].lru_position << endl;
        if (i != accessedIndex && VC[i].valid) {
            VC[i].lru_position--; // Decrement LRU position for other blocks
        }
    }

    VC[accessedIndex].lru_position = maxLRU; // Set accessed block as most recently used
}

void cache::updateL2(cacheBlock evictedBlock)
{
  int index = (evictedBlock.address >> BLOCK_OFFSET_BITS) & 0xF;
  int tag = evictedBlock.address >> (BLOCK_OFFSET_BITS + INDEX_SIZE);
  // Find a block to replace using LRU or other policy
  int replaceWay = findL2ReplacementIndex(index);
  L2[index][replaceWay] = evictedBlock;
  L2[index][replaceWay].valid = true;
  L2[index][replaceWay].tag = tag; // Make sure the tag is updated

  // Update LRU state for all blocks in this set
  // updateLRUL2(index, replaceWay);
}

int cache::findL2ReplacementIndex(int index)
{
  int replaceWay = -1;
  int maxLRUValue = -1;
  for (int i = 0; i < L2_CACHE_WAYS; i++)
  {
    if (!L2[index][i].valid)
    {
      return i; // Found an empty slot
    }
    if (L2[index][i].lru_position > maxLRUValue)
    {
      maxLRUValue = L2[index][i].lru_position;
      replaceWay = i; // Found the LRU block
    }
  }
  return replaceWay; // Return the way to be replaced
}

void cache::updateLRUL2(int index, int accessedWay)
{
  for (int i = 0; i < L2_CACHE_WAYS; i++)
  {
    if (i != accessedWay && L2[index][i].valid)
    {
      L2[index][i].lru_position--; // Decrement LRU position
    }
  }
  L2[index][accessedWay].lru_position = L2_CACHE_WAYS - 1; // Set as most recently used, 7
}

void cache::swapL1Victim(int address)
{
  int block_offset = address & 0x3;
  int index = (address >> BLOCK_OFFSET_BITS) & 0xF;              // For L1, use index to find the corresponding line
  int convert_tag = address >> (BLOCK_OFFSET_BITS + INDEX_SIZE); // Tag for L1, 26 bits
  int victimTag = address >> BLOCK_OFFSET_BITS;                  // Tag for Victim Cache, 30 bits
  // clog << "victim tag need to be matched:" << victimTag << endl;
  for (int i = 0; i < VICTIM_SIZE; i++)
  {
    if (VC[i].valid && VC[i].tag_victim == victimTag)
    {
      cacheBlock tempL1 = L1[index]; // Store L1 data

      // Update L1 cache with data from Victim Cache
      L1[index].tag = convert_tag;
      L1[index].data = VC[i].data;
      L1[index].block_offset = VC[i].block_offset;
      L1[index].valid = true;
      L1[index].address = address;

      // Prepare L1 block for Victim Cache
      cacheBlock evictedForVictim;
      if (tempL1.valid)
      {
        evictedForVictim.tag = (tempL1.tag << 4) | tempL1.index; // Convert L1 tag to 30-bit Victim Cache tag
        evictedForVictim.data = tempL1.data;
        evictedForVictim.block_offset = tempL1.block_offset;
        evictedForVictim.valid = true;
        evictedForVictim.address = tempL1.address;
      }
      else
      {
        evictedForVictim.valid = false;
      }

      // Find least recently used block in Victim Cache to replace
      int lruIndex = findVictimCacheReplacementEntry();
      if (lruIndex >= 0 && evictedForVictim.valid)
      {
        VC[lruIndex] = evictedForVictim; // Replace least recently used block
        updateLRUVictim(lruIndex);       // Update LRU state
      }
      return;
    }
  }
  cerr << "Error: Block not found in Victim Cache for swapping." << endl;
}

/*
Benchmarks calculation
*/
double cache::getL1MissRate()
{
  return ((double)myStat.missL1 / (double)myStat.accL1);
}
double cache::getL2MissRate()
{
  return ((double)myStat.missL2 / (double)myStat.accL2);
}
double cache::getVictimMissRate()
{
  return ((double)myStat.missVic / (double)myStat.accVic);
}
double cache::getAAT()
{
  double L1_hit_time = 1;
  double L2_hit_time = 8;
  double Victime_hit_time = 1;
  double MissPenaltyMem = 100;
  double AAT = L1_hit_time + getL1MissRate() * (Victime_hit_time + getVictimMissRate() * (L2_hit_time + getL2MissRate() * MissPenaltyMem));
  return (AAT);
}