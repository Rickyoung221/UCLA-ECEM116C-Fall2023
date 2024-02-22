#include "CPU.h"

instruction::instruction(bitset<32> fetch)
{
	//cout << fetch << endl;
	instr = fetch;
	//cout << instr << endl;
}

//constructor class for CPU
CPU::CPU()
{
	PC = 0; //initializes the Program Counter to 0.
	r_type_count = 0;
	instruction_count = 0; //IC
	total_clock_cycles = 0; 
	instruction_per_cycle = 0; //IPC

	for (int i = 0; i < 4096; i++) //copy instrMEM
	{
		dmemory[i] = (0); //An array of 4096 elements, which represents the data memory. It initializes all elements to 0.
	}
	for (int j = 0; j < 32; j++)
	{
		register_file[j] = (0); // An array of 32 elements, which represents the register file. It initializes all elements to 0s.
	}

bitset<32> CPU::Fetch(bitset<8> *instmem) {
	//responsible for fetching an instruction from memory. It takes an array of bitset<8> (which presumably represents memory) as input.
	//combines four bytes (8 bits each) from the memory array to create a 32-bit instruction.
	bitset<32> instr = ((((instmem[PC + 3].to_ulong()) << 24)) + ((instmem[PC + 2].to_ulong()) << 16) + ((instmem[PC + 1].to_ulong()) << 8) + (instmem[PC + 0].to_ulong()));  //get 32 bit instruction
	PC += 4;//increment PC to next instruction
	return instr; //returns the 32-bit instruction that was fetched.
}

unsigned long CPU::readPC()
{
	return PC;
}

// Add other functions here ... 

bool CPU::decode(instruction* curr)
{
	//identify opcode -> fun3 -> fun7 
	unit32_t curr_instr; /* 32-bit value instruction currently */;
	curr_instr = (uint32_t)(curr -> instr.to_ulong());

	// extract the lowest 7 bits in instruction which is the opcode
	uint32_t opcode = curr_instr & 0x7F;
	// right shift 12 bit and extract the lowest 3 bit 
	uint32_t fun3 = (curr_instr >> 12) & 0x7;
	// right shift 25 bit and extract the loewest 7 bit.
	uint32_t fun3 = (curr_instr >> 25) & 0x7F;
	// register number, extract 5 bits after shifted. 
	uint32_t rs1 = (curr_instr >> 15) & 0x1f;
	uint32_t rs2 = (curr_instr >> 20) & 0x1f;

	if (opcode == RTYPE)
	{
		if 
	}



//cout<<curr->instr<<endl;
return true;
}

void CPU::execute() {
    // Execute implementation
    // Update executeInstr
}

void CPU::memory() {
    // Memory implementation
    // Update memInstr
}

void CPU::writeback()
{
}



void CPU::printRegisters() {
    // Print the registers' state
}
