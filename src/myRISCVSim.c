
/* 

The project is developed as part of Computer Architecture class
Project Name: Functional Simulator for subset of RISCV Processor

Developer's Name:
Developer's Email id:
Date: 

*/


/* myRISCVSim.cpp
   Purpose of this file: implementation file for myRISCVSim
*/

#include "myRISCVSim.h"
#include <stdlib.h>
#include <stdio.h>

//Register file
static unsigned int X[32];
//flags
//memory
static unsigned char MEM[4000];

//intermediate datapath and control path signals
static unsigned int instruction_register;
static unsigned int operand1;
static unsigned int operand2;




static unsigned int regdestiny;
static unsigned int Swreg;


// For Opcode.
#define Rtype 51
#define ItypeA 19
#define ItypeL 3
#define ItypeJ 103
#define Stype 35
#define Btype 99
#define Jtype 111
#define UtypeL 23
#define UtypeA 55


// For Op2.
#define OP2_from_RF 0
#define OP2_Imm 1
#define OP2_ImmS 2


// For IsBranch.
#define NoBranch 0
#define Branched 1
#define Branch_with_ALU 2

// For ResultSelect.
#define from_ALU 0
#define from_ImmU 1
#define from_MEM 2
#define from_PC 3

// For RFwrite
#define No_RF 0
#define Yes_RF 1


// All control signals.
typedef struct{
	unsigned int ALUOp;
	unsigned int IsBranch;
	unsigned int MemOp;
	unsigned int Memon;
	unsigned int Op2select;
	unsigned int ResultSelect;
	unsigned int BranchTarget;
	unsigned int RFwrite;
}controls;


unsigned int extract_bits(int low, int high)


unsigned int instruction_type;
unsigned int PC = 0;







void run_riscvsim() {
  while(1) {
    fetch();
    decode();
    execute();
    mem();
    write_back();
  }
}

// it is used to set the reset values
//reset all registers and memory content to 0
void reset_proc() {
	for(int i=0; i<32; i++){
		X[i]=0;
	}
	for(int i=0; i<4000; i++){
		MEM[i] = 0;
	}

	controls.ALUOp = -1;
	controls.ISBranch = -1;
	controls.MemOp = -1;
	controls.Memon = -1;
	controls.Op2select = -1;
	controls.ResultSelect = -1;
	controls.BranchTarget = -1;
	controls.RFwrite = -1;
}

//load_program_memory reads the input memory, and pupulates the instruction 
// memory
void load_program_memory(char *file_name) {
  FILE *fp;
  unsigned int address, instruction;
  fp = fopen(file_name, "r");
  if(fp == NULL) {
    printf("Error opening input mem file\n");
    exit(1);
  }
  while(fscanf(fp, "%x %x", &address, &instruction) != EOF) {
    write_word(MEM, address, instruction);
  }
  fclose(fp);
}

//writes the data memory in "data_out.mem" file
void write_data_memory() {
  FILE *fp;
  unsigned int i;
  fp = fopen("data_out.mem", "w");
  if(fp == NULL) {
    printf("Error opening dataout.mem file for writing\n");
    return;
  }
  
  for(i=0; i < 4000; i = i+4){
    fprintf(fp, "%x %x\n", i, read_word(MEM, i));
  }
  fclose(fp);
}

//should be called when instruction is swi_exit
void swi_exit() {
  write_data_memory();
  exit(0);
}


//reads from the instruction memory and updates the instruction register
void fetch() {
	instruction_register = *((unsigned int*)&MEM[PC]);
}

//reads the instruction register, reads operand1, operand2 fromo register file, decides the operation to be performed in execute stage
void decode() {
	unsigned int func3, rs1, rs2, rd, opcode;	

	opcode = extract_bits(0,6,0);

	rs1 = extract_bits(15,19,0);	
	rd = extract_bits(7,11,0);
	rs2 = extract_bits(20,24,0);



	int imm = extract_bits(20,31,1);
	int immS = (extract_bits(25,31,1) << 5) + extract_bits(7,11,1);
		
	

	switch(opcode){
	
	// R-type
	case(Rtype){
		controls.Op2select = Op2_from_RF;
		controls.Memon = 0;
		controls.IsBranch = NoBranch;
		controls.ResultSelect = from_ALU;	
		controls.RFwrite = 1;		

		func3 = extract_bits(12,14,0);
		controls.ALUOp = func3;

		unsigned int func7 = extract_bits(25,31,0);
		if(!(func7 && 1<<5)){
			controls.ALUOp += func7;	
		}
		
		operand1 = X[rs1];	
		regdestiny = rd;
		break;
	}

	// I-type - JALR
	case(ItypeJ){
		controls.ALUOp = 0;
	}
	
	// I-type - load
	case(ItypeL){
		controls.ALUOp = 0;
	}

	// I-type - Arithmetic
	case(ItypeA){
		controls.Op2select = Op2_Imm;

		func3 = extract_bits(12,14,0);
		controls.ALUOp = func3;
	
		operand1 = X[rs1];
		break;
	}
		
	// S-type
	case(Stype){
		controls.Op2select = Op2_Imms;
		controls.ALUOp = 0;
		controls.Memon =1;

		operand1 = X[rs1];
		Swreg = rs2;

	}
	
	// B-type
	case(Btype){
		instruction_type = 5;
	}
		
	// J-type
	case(Jtype){
		instruction_type = 6;
	}
	
	// U-type - auipc
	case(UtypeA){
		instruction_type = 7;
	}

	// U-type - lui
	case(UtypeB){
		instruction_type = 8
	}
		
	// Selects OP2 for ALU.
	switch(controls.Op2select){
		case(Op2_from_RF){
			operand2 = X[rs2];	
		}
		case(Op2_Imm){
			operand2 = imm;
		}
		case(Op2_Imms){
			operand2 = imms;
		}
	}
}
	
}
//executes the ALU operation based on ALUop
void execute() {
}
//perform the memory operation
void mem() {
}
//writes the results back to register file
void write_back() {
}


int read_word(char *mem, unsigned int address) {
  int *data;
  data =  (int*) (mem + address);
  return *data;
}

void write_word(char *mem, unsigned int address, unsigned int data) {
  int *data_p;
  data_p = (int*) (mem + address);
  *data_p = data;
}




int extract_bits(int low, int high, int sign){
	if(!sign){
		unsigned int foo = instruction_register;
	}	
	else{
		int foo = instruction_register;
	}
	foo << 31 - high;
	foo >> 31+low-high;
	return (int)foo;
}
