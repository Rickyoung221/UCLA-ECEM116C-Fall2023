#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

// define opcode types


class instruction {
public:
	bitset<32> instr;//instruction
	instruction(bitset<32> fetch); // constructor

};

enum class Op {
    OTHER, ADD, SUB, ADDI, XOR, ANDI, SRA, LW, SW, BLT, JALR
};

class CPU {
private: //Members declared under the private access specifier are not accessible from outside the class
	 // Define opcode constants
	static const uint8_t RTYPE = 0x33; /// add, sub, xor, SRA 0110011
    static const uint8_t ITYPE = 0x13; // Addi, AndI 0010011
    static const uint8_t LOADWORD = 0x03; // 0000011
    static const uint8_t STOREWORD = 0x23; //0100011
    static const uint8_t BTYPE = 0x63; //1100111 BLT
    static const uint8_t JALR = 0x67; // 1100111

	map<uint8_t, Op> opcodeMap;

	int dmemory[4096]; //data memory byte addressable in little endian fashion;
	unsigned long PC; //pc 
	int32_t register_file[32]; //32 register files x0-x31
	// keeps track of performance metrics
	int instruction_count; //IC
	int total_clock_cycles; 
	double instruction_per_cycle; //IPC
	int r_type_count;

	//initite 
	struct RiscVDecode{
		int32_t rs1 = 0;
		int32_t rs2 = 0;
		ut32_t rd = 0;
	} decodeInstr;


public: //Members declared under the public access specifier are accessible from outside the class.
	CPU();
	//A function to read the current value of the program counter (PC).
	unsigned long readPC();
	//A function responsible for fetching an instruction from memory (given an array of instruction memory represented as bitsets)
	bitset<32> Fetch(bitset<8> *instmem);
	// It takes an instruction pointer as input and returns a boolean indicating whether the instruction was successfully decoded.
	bool Decode(instruction* instr);
	// each state
	void execute();
	void memory();
	void writeback();
	// print
	void printRegisters();
	
};

// add other functions and objects here
