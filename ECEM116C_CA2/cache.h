#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1
#define BLOCK_OFFSET_BITS 2 //for all cache, the same size
#define INDEX_SIZE 4 //for DM and SA

struct cacheBlock
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	int data; // the actual data stored in the cache/memory
	bool valid;
	// add more things here if needed
	int index;
    int address;
    int block_offset;
    int tag_victim;
};

struct Stat
{
	int missL1; 
	int missL2;
	int hitL1;
	int hitL2;
	int accL1;
	int accL2;
	int accVic;
	int missVic;
	int hitVic;
	// add more stat if needed. Don't forget to initialize!
};

class cache {
private:
    cacheBlock L1[L1_CACHE_SETS];
    cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS];
    cacheBlock VC[VICTIM_SIZE];
    Stat myStat;
    //search
    bool searchL1(int index, int tag, int &data);
    bool searchVictimCache(int tag, int &data);
    bool searchL2(int index, int tag, int &data);
    int loadFromMemory(int address, int* myMem);
    //update
    cacheBlock updateL1(int index, int tag, int data, int block_offset, int tag_victim, int address);
    cacheBlock updateVictimCache(cacheBlock newBlock);
    void updateL2(cacheBlock evictedBlock);
    //evcit policy and update LRU
    void swapL1Victim(int address);
    int findVictimCacheReplacementEntry();
    void updateLRUVictim(int accessedIndex);
    int findL2ReplacementIndex(int index);
    void updateLRUL2(int index, int accessedWay);

public:
    cache();
    void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
    int loadWord(int block_offset, int tag, int index, int tag_victim, int address, int* myMem);
    void storeWord(int block_offset, int tag, int index, int address, int tag_victim, int data);
    //benchmarks
    double getL1MissRate();
    double getL2MissRate();
    double getVictimMissRate();
    double getAAT();
};