import sys

class Instruction:
    def __init__(self, fetch):
        self.instr = fetch

class OpcodeType:
    RTYPE = 0x33  # add, sub, xor, SRA (0110011)
    ITYPE = 0x13  # ADDI, ANDI (0010011)
    LOAD = 0x03  # LW (0000011)
    STYPE = 0x23  # SW (0100011)
    BTYPE = 0x63  # BLT (1100011)
    JALR = 0x67  # JALR (1100111)
    ZERO= 0x00 # terminate opcode

class CPU:
    def __init__(self):
        self.dataMemory = [0] * 4096  # Data memory byte addressable in little endian fashion
        self.PC = 0  # PC
        self.registerFile = [0] * 32  # 32 Registers x0-x31
        self.totalClockCycles = 0   
        self.instructionCount = 0   #IC
        self.instrucitonPerCycle = 0.0 #IPC
        self.rtypeCount = 0

        self.decoded_instruction = {
            "rs1": 0,
            "rs2": 0,
            "rd": 0,
            "immediate": 0,
            "operation": None
        }

        self.execute_instruction = {
            "aluResult": 0,
            "rs2": 0,
            "rd": 0,
            "operation": None
        }

        self.memory_instruction = {
            "rd": 0,
            "aluResult": 0,
            "dataMem": 0,
            "operation": None
        }

    def readPC(self):
        return self.PC

    '''
    1. R-Type: ADD, SUB, XOR, SRA
    2. I-Type: ANDI, ADDI 
    3. LOAD: LW
    4. STYPE: SW
    5. Branch-Type: BLT
    6. JALR
    7. ERROR: Error handing for other instructions
    8. ZERO: ternimal instruction 
    '''

    def print_result(self):
        a0 = self.registerFile[10]  # x10
        a1 = self.registerFile[11]  # x11
        print(f"({a0}, {a1})")
        self.instrucitonPerCycle = float(self.instructionCount) / self.totalClockCycles

        #print("Number of R-type instructions:", self.rtypeCount)
        #print("Number of clock cycles:", self.totalClockCycles)
        #print("Number of instructions:", self.instructionCount)
        #print("IPC:", self.instrucitonPerCycle)

def fetch_stage(cpu, instmem):
    instr = (
        (instmem[cpu.PC + 3] << 24)
        + (instmem[cpu.PC + 2] << 16)
        + (instmem[cpu.PC + 1] << 8)
        + instmem[cpu.PC]
    )
    # Default: next instruction
    cpu.PC += 4
    return instr

def decode_stage(cpu, current_instruction):
    '''
    Decode the fetched instruction and extract relevant information.
    '''
    # Identify opcode -> fun3 -> fun7
    # Extract the lowest 7 bits in instruction which is the opcode
    opcode = current_instruction & 0x7F
    # Right shift 12 bits and extract the lowest 3 bits
    func3 = (current_instruction >> 12) & 0x7
    # Right shift 25 bits and extract the lowest 7 bits
    func7 = (current_instruction >> 25) & 0x7F
    # Register number, extract 5 bits after shifted.
    rs1 = (current_instruction >> 15) & 0x1F
    rs2 = (current_instruction >> 20) & 0x1F
    rd = (current_instruction >> 7) & 0x1F
    # Immediate 12 bits
    immediate = current_instruction >> 20

    # Store decoded information in the CPU object
    cpu.decoded_instruction["rs1"] = cpu.registerFile[rs1]
    cpu.decoded_instruction["rs2"] = cpu.registerFile[rs2]
    cpu.decoded_instruction["rd"] = rd
    cpu.decoded_instruction["immediate"] = immediate

    # Distinguish the operation type based on opcode and func3/func7
    if opcode == OpcodeType.RTYPE:
        if func3 == 0x0:
            if func7 == 0x00:
                cpu.decoded_instruction["operation"] = "ADD"
            elif func7 == 0x20:
                cpu.decoded_instruction["operation"] = "SUB"
            else:
                cpu.decoded_instruction["operation"] = "ERROR"
        elif func3 == 0x4 and func7 == 0x0:
            cpu.decoded_instruction["operation"] = "XOR"
        elif func3 == 0x5 and func7 == 0x20:
            cpu.decoded_instruction["operation"] = "SRA"
        else:
            cpu.decoded_instruction["operation"] = "ERROR"
        # Detect R-type, count it
        cpu.rtypeCount += 1

    elif opcode == OpcodeType.ITYPE:
        cpu.decoded_instruction["immediate"] = current_instruction >> 20
        imm12 = cpu.decoded_instruction["immediate"] >> 11
        if imm12 == 1: # assume it is 1,
            cpu.decoded_instruction["immediate"] |= 0xFFFFF000
            cpu.decoded_instruction["immediate"] = -((~cpu.decoded_instruction["immediate"]+1)& 0xFFFFFFFF)
            #print(str(hex(cpu.decoded_instruction["immediate"])))
            #cpu.decoded_instruction["immediate"]= -((~cpu.decoded_instruction["immediate"] + 1) & 0xFFFFFFFF)
            #print(str(hex(cpu.decoded_instruction["immediate"])))
            #print(int(str(hex(cpu.decoded_instruction["immediate"])), 16))
        if func3 == 0x0:
            cpu.decoded_instruction["operation"] = "ADDI"
        elif func3 == 0x7:
            cpu.decoded_instruction["operation"] = "ANDI"
        else:
            cpu.decoded_instruction["operation"] = "ERROR"

    elif opcode == OpcodeType.LOAD:
        if func3 == 0x2:
            cpu.decoded_instruction["operation"] = "LW"
        else:
            cpu.decoded_instruction["operation"] = "ERROR"    

    elif opcode == OpcodeType.JALR and func3 == 0x0:
        cpu.decoded_instruction["operation"] = "JALR"

    elif opcode == OpcodeType.STYPE:
        if func3 == 0x2:  # SW
            # The 32nd bit to the 26th bit
            imm11_5 = (current_instruction & 0xFE000000)
            # The twelfth bit - the 8th bit
            # Use AND gate with 1 to extract the bit we want; any number & 1 would be the original.
            # Left shift 13 bits to align the extracted bits to correct positions to combine with imm11_5,
            # correctly construct the immediate value.
            imm4_0 = (current_instruction & 0xF80) << 13
            # Right shifting by 20 bits to obtain the final immediate value (MSB 12 bits)
            # Parsing the value and store
            cpu.decoded_instruction["immediate"] = (imm11_5 | imm4_0) >> 20
            cpu.decoded_instruction["operation"] = "SW"
            
        else:
            cpu.decoded_instruction["operation"] = "ERROR"
    elif opcode == OpcodeType.BTYPE:
        if func3 == 0x4:  # BLT
            # Extracting individual bit fields
            # 13-bit number, with the LSB as 0 due to left shift 1 bit
            imm0 = 0  # LSB is always 0 for branch instruction
            imm4_1 = (current_instruction & 0xF00) >> 7
            imm10_5 = (current_instruction & 0x7E000000) >> 20
            imm11 = (current_instruction & 0x80) << 4  # In instruction [7], move to the 12th bit
            imm12 = (current_instruction & 0x80000000) >> 19  # 32 - 13 = 19

            # Combine the extracted bits
            cpu.decoded_instruction["immediate"] = imm12 | imm11 | imm10_5 | imm4_1 | imm0

            # Sign-extend the branch offset to 32 bits
            # If imm12 (MSB) is set to 1, 
            if immediate & 0x800:
                cpu.decoded_instruction["immediate"] |= 0xFFFFF000

            cpu.decoded_instruction["operation"] = "BLT"          
        else:
            cpu.decoded_instruction["operation"] = "ERROR"
    # TERMINAL, no operation
    elif opcode == OpcodeType.ZERO:
        cpu.decoded_instruction["operation"] = "ZERO"

    else:
        print("Wrong Instruction! This instruction is not included in this project spec: ", opcode)
        cpu.decoded_instruction["operation"] = "ERROR"
    cpu.instructionCount += 1
    return True      


def execute_stage(cpu):
    '''
    Execute the ALU operation based on the decoded instruction and update relevant information.
    '''
    # Copy relevant information from the decoded instruction
    rs1 = cpu.decoded_instruction["rs1"]
    rs2 = cpu.decoded_instruction["rs2"]
    operation = cpu.decoded_instruction["operation"]
    immediate = cpu.decoded_instruction["immediate"]

    execute_instruction = {
        "operation": cpu.decoded_instruction["operation"],
        "rs2": cpu.decoded_instruction["rs2"],
        "rd": cpu.decoded_instruction["rd"],
        "aluResult": 0
    }

    # Perform the ALU operation based on the instruction type
    if operation == "ADD":
        execute_instruction["aluResult"] = rs1 + rs2
    elif operation == "SUB":
        execute_instruction["aluResult"] = rs1 - rs2
    elif operation == "SRA":
        # arithmetic shift, keep the sign-bit
        sign_bit = rs1 & 0x80000000
        abs_value = abs(rs1)  # abs value of rs1
        # eusure that shifting by 31 positions.
        shift_value = abs_value >> (rs2 & 0x1F)
        if sign_bit == 1:
            shift_value = shift_value | (0xFFFFFFFF << (32 - (rs2 & 0x1F)))
        execute_instruction["aluResult"] = shift_value
    elif operation == "XOR":
        execute_instruction["aluResult"] = rs1 ^ rs2
    elif operation == "ADDI":
        execute_instruction["aluResult"] = rs1 + immediate
    elif operation == "ANDI":
        execute_instruction["aluResult"] = rs1 & immediate
    elif operation == "LW" or operation == "SW":
        effective_address = rs1 + immediate
        #if effective_address % 4 != 0:
         #   raise Exception("Unaligned memory access")
        if effective_address < 0 or effective_address >= len(cpu.dataMemory):
            raise Exception("Memory access out of bounds")
        execute_instruction["aluResult"] = effective_address

    elif operation == "BLT":
        if rs1 < rs2:
            # Pc = Pc + imm
            cpu.PC = cpu.PC + (immediate & ~1) - 4
    elif operation == "JALR":
        # pc = rs1 + imm
        execute_instruction["aluResult"] = cpu.PC
        # AND with ~1: clears the least significant bit, ensuring word alignment. the address must be even
        cpu.PC = (rs1 + immediate) & ~1
    elif operation == "ZERO":
        # No operation. add x0, x0, 0
        execute_instruction["aluResult"] = 0

    return execute_instruction

def memory_stage(cpu):
    '''
    For load and store instruction, only LW and SW use this stage among the instruction set
    '''
    memory_instruction = {
        "rd": cpu.execute_instruction["rd"],
        "aluResult": cpu.execute_instruction["aluResult"],
        "operation": cpu.execute_instruction["operation"],
    }

    addr = cpu.execute_instruction["aluResult"]
    rs2 = cpu.execute_instruction["rs2"]
    operation = cpu.execute_instruction["operation"]

    # 4 bytes, stored into dataMem field
    # rd = M[rs1+imm]
    if operation == "LW":
        # if addr % 4 == 0:
        # Aligned memory access; read a single 32-bit word
        word = (
            (cpu.dataMemory[addr + 3] << 24)
            | (cpu.dataMemory[addr + 2] << 16)
            | (cpu.dataMemory[addr + 1] << 8)
            | cpu.dataMemory[addr]
        )
        memory_instruction["dataMem"] = word
        # else:
        # Unaligned memory access; read four bytes separately
        # raise ValueError("Unaligned memory access for LW instruction")
    elif operation == "SW":
        # if addr % 4 == 0:
        # Aligned memory access; write a single 32-bit word
        cpu.dataMemory[addr + 3] = (rs2 >> 24) & 0xFF
        cpu.dataMemory[addr + 2] = (rs2 >> 16) & 0xFF
        cpu.dataMemory[addr + 1] = (rs2 >> 8) & 0xFF
        cpu.dataMemory[addr] = rs2 & 0xFF
        # else:
        # Unaligned memory access;
        # raise ValueError("Unaligned memory access for SW instruction")

    return memory_instruction

def write_back(cpu):
    cpu.totalClockCycles += 1

    if cpu.memory_instruction["rd"] == 0 or cpu.memory_instruction["operation"] == "SW":
        return
    if cpu.memory_instruction["operation"] == "JALR":
        cpu.registerFile[cpu.memory_instruction["rd"]] = cpu.execute_instruction["aluResult"]

    elif cpu.memory_instruction["operation"] == "LW":
        cpu.registerFile[cpu.memory_instruction["rd"]] = cpu.memory_instruction["dataMem"]
    elif cpu.memory_instruction["operation"] != "ERROR":
        cpu.registerFile[cpu.memory_instruction["rd"]] = cpu.memory_instruction["aluResult"]



if __name__ == "__main__":
    '''
    This is the front end of your project.
	You need to first read the instructions that are stored in a file and load them into an instruction memory.

	Each cell should store 1 byte. You can define the memory either dynamically, or define it as a fixed size with size 4KB (i.e., 4096 lines). Each instruction is 32 bits (i.e., 4 lines, saved in little-endian mode).
	Each line in the input file is stored as an unsigned char and is 1 byte (each four lines are one instruction). You need to read the file line by line and store it into the memory. You may need a mechanism to convert these values to bits so that you can read opcodes, operands, etc.
	'''
    # bitset<8> instMem[4096];
    inst_mem = [0] * 4096

    if len(sys.argv) < 2:
        print("No file name entered. Exiting...")
        sys.exit(1)

    try:
        # open the file
        with open(sys.argv[1], "r") as infile:
            i = 0
            for line in infile:
                x = int(line.strip())  # Read integers directly from the file
                inst_mem[i] = x
                i += 1
    except Exception as e:
        print("Error opening file:", e)
        # exiting with an error
        sys.exit(1)

    max_PC = i
    '''
    call the approriate constructor here to initialize the processor...  
	make sure to create a variable for PC and resets it to zero (e.g., unsigned int PC = 0); 
	'''
    myCPU = CPU()

    instr = 0 
    done = True
    while done: # processor's main loop. Each iteration is equal to one clock cycle.  
        # fetch
        
        instr = fetch_stage(myCPU, inst_mem)

        # decode
        done = decode_stage(myCPU, instr)
        if not done: # break from loop so stats are not mistakenly updated
            break

        # start, other stages
        # exectuion
        myCPU.execute_instruction = execute_stage(myCPU)

        # memory
        myCPU.memory_instruction = memory_stage(myCPU)

        # write back
        write_back(myCPU)

        # sanity check
        if myCPU.readPC() > max_PC:
            break
        #a0 = 0
        #a1 = 0

    # Print the results
    myCPU.print_result()
