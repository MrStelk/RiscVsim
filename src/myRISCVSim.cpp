
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
#include "iostream"
#include "myRISCVSim.h"
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <math.h>

using namespace std;

//Register file
static int X[32];
//flags
//memory
static unsigned char MEM[4000000];
map<int , unsigned char > DMEM;

//intermediate datapath and control path signals
static unsigned int instruction_register;
static int operand1;
static int operand2;





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
#define EXIT 32

// For Op2.
#define Op2_RF 0
#define Op2_Imm 1
#define Op2_ImmS 2


// For IsBranch.
#define NoBranch 0
#define Branched 1
#define Branch_From_ALU 2

// For ResultSelect.
#define From_ALU 0
#define From_ImmU 1
#define From_MEM 2
#define From_PC 3
#define From_AUIPC 4

// For RFWrite
#define NoWrite 0
#define Write 1

// For MemOp
#define NoMEMOp 0
#define MEM_sw 1
#define MEM_sh 2
#define MEM_sb 3
#define MEM_lw 4
#define MEM_lh 5
#define MEM_lb 6

// For ALUOp
#define Add_op 0
#define Sll_op 1
#define	Slt_op 2
#define Xor_op 4
#define Srl_op 5 
#define Or_op 6 
#define And_op 7
#define	Sub_op 32
#define Sra_op 37

// For BranchTarget.
#define Branch_ImmJ 0
#define Branch_ImmB 1


// For BeanchType.
#define BEQ 0
#define BNE 1
#define BGE 2
#define BLT 3


// All control signals.
struct{
	unsigned int ALUOp;
	unsigned int IsBranch;
	unsigned int MemOp;
	unsigned int Op2Select;
	unsigned int ResultSelect;
	unsigned int BranchTarget;
	unsigned int RFWrite;
	unsigned int BranchType;
}controls;


unsigned int extract_bits(int low, int high);
unsigned int extract_byte(int low, int value);
int sign_extender(int num, int MSB);
void viewMEM(int low, int a);

unsigned int BranchTarget_Addr;
unsigned int instruction_type;
unsigned int PC = 0;
static unsigned int regdestiny;
static unsigned int SwOp2;
int ALUresult;
int Loaded_Data;
int Imm_U;
int Imm_B;
int Imm_J;


void run_riscvsim() {
  while(1) {
  	cout << "\n\n---------New cycle---------\n";
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
	controls.IsBranch = -1;
	controls.MemOp = -1;
	controls.Op2Select = -1;
	controls.ResultSelect = -1;
	controls.BranchTarget = -1;
	controls.RFWrite = -1;
	controls.BranchType = -1;
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
    write_word(&MEM[0], address, instruction);
  }
  viewMEM(0,8);
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
    fprintf(fp, "%x %x\n", i, read_word(&MEM[0], i));
  }
  fclose(fp);
}

//should be called when instruction is swi_exit
void swi_exit() {
//  write_data_memory();
  exit(0);
}


//reads from the instruction memory and updates the instruction register
void fetch() {
	cout << "ssFetch:\n";
	instruction_register = *((unsigned int*)&MEM[PC]);
	cout << "		instruction_register : ";
	printf("%x\n", instruction_register);
	cout << "		PC:" << PC<<endl;
}

//reads the instruction register, reads operand1, operand2 fromo register file, decides the operation to be performed in execute stage
void decode() {
	cout << "\n\nDecode:\n";
	unsigned int func3, rs1, rs2, rd, opcode;	

	opcode = extract_bits(0,6);cout <<"		opcode:"<<opcode<<endl;

	rs1 = extract_bits(15,19);	
	regdestiny = rd = extract_bits(7,11);
	rs2 = extract_bits(20,24);


	int imm = extract_bits(20, 31);
	imm = sign_extender(imm, 11);
		
	int immS = extract_bits(25,31) << 5;
	immS += extract_bits(7,11);
	immS = sign_extender(immS, 11);
	
	int immB = (extract_bits(31,31) << 12);
	immB += (extract_bits(7,7) << 11);
	immB += (extract_bits(25,30) << 5);
	immB += (extract_bits(8,11) << 1);
	immB = sign_extender(immB, 12);
	Imm_B = immB;


	int immU = extract_bits(12,31);
	immU = sign_extender(immU, 19) << 12;
	Imm_U = immU;

	int immJ = extract_bits(31,31) << 20;
	immJ += (extract_bits(12,19) << 13);
	immJ += (extract_bits(20,20) << 12);	
	immJ += (extract_bits(21,30) << 1);
	immJ = sign_extender(immJ, 20);
	Imm_J = immJ;
	

	switch(opcode){
		// R-type
		case(Rtype):{
			controls.Op2Select = Op2_RF;
			controls.MemOp = NoMEMOp;
			controls.IsBranch = NoBranch;
			controls.ResultSelect = From_ALU;	
			controls.RFWrite = Write;		
	
			func3 = extract_bits(12,14);
			controls.ALUOp = func3;
	
			unsigned int func7 = extract_bits(25,31);
			if(func7==32){
				controls.ALUOp += func7;	
			}
			
			operand1 = X[rs1];	
			regdestiny = rd;
			break;
		}	

		// I-type - JALR
		case(ItypeJ):{
			controls.ALUOp = 0;
			controls.Op2Select = Op2_Imm;
			controls.ResultSelect = From_PC;
			controls.MemOp = NoMEMOp;
			controls.RFWrite = Write;
			controls.IsBranch = Branch_From_ALU;
			controls.BranchTarget = Branch_ImmJ;\
			break;
		}
	
		// I-type - load
		case(ItypeL):{
			controls.ALUOp = 0;
			controls.Op2Select = Op2_Imm;	
			controls.ResultSelect = From_MEM;
			controls.RFWrite = Write;
			controls.IsBranch = NoBranch;

			func3 = extract_bits(12,14);	
			switch(func3){
				case(0):{
					controls.MemOp = MEM_lb;
					break;
				}
				case(1):{
					controls.MemOp = MEM_lh;
					break;
				}
				case(2):{
					controls.MemOp = MEM_lw;
					break;
				}
			}
			break;
		}	
	
		// I-type - Arithmetic
		case(ItypeA):{
			controls.Op2Select = Op2_Imm;
			controls.MemOp = NoMEMOp;
			controls.ResultSelect = From_ALU;
			controls.RFWrite = Write;
			controls.IsBranch = NoBranch;

			func3 = extract_bits(12,14);
			controls.ALUOp = func3;
			cout << "		Fun3:" << func3 << endl;
			operand1 = X[rs1];
			break;
		}
		
		// S-type
		case(Stype):{
			controls.Op2Select = Op2_ImmS;
			controls.ALUOp = 0;
			controls.RFWrite = NoWrite;
			controls.IsBranch = NoBranch;

			func3 = extract_bits(12,14);	
			switch(func3){
				case(0):{
					controls.MemOp = MEM_sb;
					break;
				}
				case(1):{
					controls.MemOp = MEM_sh;
					break;
				}
				case(2):{
					controls.MemOp = MEM_sw;
					break;
				}
			}

			operand1 = X[rs1];
			SwOp2 = X[rs2];
			break;
		}
	
		// B-type
		case(Btype):{
			operand1=X[rs1];
			operand2=X[rs2];
			func3 = extract_bits(12,14);
			controls.ALUOp=32;
			controls.Op2Select= Op2_RF;
			controls.RFWrite= NoWrite;
			controls.IsBranch= Branched;
			controls.BranchTarget= Branch_ImmB;
			controls.BranchType=func3;
        		controls.MemOp=NoMEMOp;
			break;
		}
		
		// J-type
		case(Jtype):{
			controls.RFWrite= Write;
			controls.ResultSelect= From_PC;
			controls.BranchTarget= Branch_ImmJ;
			controls.IsBranch= Branched;
			controls.MemOp=NoMEMOp;
			break;
		}
	
		// U-type - auipc
		case(UtypeA):{
			controls.RFWrite= Write;
			controls.ResultSelect= From_AUIPC;
			controls.ALUOp=0;
			controls.IsBranch=NoBranch;
			controls.MemOp=NoMEMOp;
			break;
		}

		// U-type - lui
		case(UtypeL):{
			controls.RFWrite= Write;
			controls.ResultSelect= From_ImmU;
			controls.MemOp= NoMEMOp;
			controls.IsBranch=NoBranch;
			break;
		}
		
		// EXIT
		case(EXIT):{
			swi_exit();	
		}
	}

	// Selects OP2 for ALU.
	switch(controls.Op2Select){
		case(Op2_RF):{
			operand2 = X[rs2];
			break;	
		}
		case(Op2_Imm):{
			operand2 = imm;
			break;
		}
		case(Op2_ImmS):{
			operand2 = immS;
			break;
		}
	}
	cout << "		operand1:" << operand1<<endl;
	cout << "		operand2:" << operand2 << endl;
}

//executes the ALU operation based on ALUop
void execute(){
	cout << "\n\nExecute:\n";
	int add,sub,_xor,_or,_and,sll,sra,slt,sltu;
	unsigned int srl;

	add = operand1 + operand2;
	sub = operand1 - operand2;
	_xor = operand1 ^ operand2;
	_or = operand1 | operand2;
	_and = operand1 & operand2;
	sll = operand1 << operand2;
	srl = (unsigned int)operand1 >> operand2;
	sra = operand1 >> operand2;
	slt = (operand1 < operand2)? 1 : 0 ;
	
	cout << "		ALUop:" << controls.ALUOp << endl;
	switch(controls.ALUOp){
		case(Add_op):{
			ALUresult = add;
			break;
		}
		case(Sub_op):{
			ALUresult = sub;
			break;
		}
		case(Xor_op):{
			ALUresult = _xor;
			break;
		}
		case(Or_op):{
			ALUresult = _or;
			break;
		}
		case(And_op):{
			ALUresult = _and;
			break;
		}
		case(Sll_op):{
			ALUresult = sll;
			break;
		}
		case(Srl_op):{
			ALUresult = srl;
			break;
		}
		case(Sra_op):{
			ALUresult = sra;
			break;
		}
		case(Slt_op):{
			ALUresult = slt;
			break;
		}
	}
	cout << "		ALUresult:" << ALUresult<<endl;
	if(controls.BranchTarget == Branch_ImmJ){
		BranchTarget_Addr = Imm_J + PC;	
	}
	else if (controls.BranchTarget == Branch_ImmB){
		BranchTarget_Addr = Imm_B + PC;
	}
	if(controls.IsBranch == Branched){
		switch(controls.BranchType){
			case(BEQ):{
				if(ALUresult){
					controls.IsBranch = NoBranch;
				}
				break;
			}
			case(BNE):{
				if(!ALUresult){
					controls.IsBranch = NoBranch;
				}
				break;
			}
			case(BGE):{
				if(ALUresult & (1<<31)){
					controls.IsBranch = NoBranch;		
				}
				break;
			}
			case(BLT):{
				if(!(ALUresult & (1<<31))){
					controls.IsBranch = NoBranch;
				}
				break;
			}
		}
	}
}


//perform the memory operation
void mem() {
	cout << "\n\nMemory:\n";
	cout << "		MemOp: " << controls.MemOp << endl;
	if(controls.MemOp != NoMEMOp){
		
		switch(controls.MemOp){
			case(MEM_lb):{
				Loaded_Data = DMEM[ALUresult];
				printf(" loaded : %x",Loaded_Data);
				break;
			}
			case(MEM_lh):{
				Loaded_Data = DMEM[ALUresult+1];
				Loaded_Data = Loaded_Data << 8;
				Loaded_Data = Loaded_Data + DMEM[ALUresult];
				printf(" loaded : %x",Loaded_Data);
				break;
			}
			case(MEM_lw):{
				Loaded_Data = DMEM[ALUresult+3];
				Loaded_Data = Loaded_Data << 8;
				Loaded_Data = Loaded_Data + DMEM[ALUresult+2];
				Loaded_Data = Loaded_Data << 8;
				Loaded_Data = Loaded_Data + DMEM[ALUresult+1];
				Loaded_Data = Loaded_Data << 8;
				Loaded_Data = Loaded_Data + DMEM[ALUresult];
				printf(" loaded : %x",Loaded_Data);
				break;
			}		
			case(MEM_sw):{
				DMEM[ALUresult] = extract_byte(0,SwOp2);
				DMEM[ALUresult+1] = extract_byte(8,SwOp2);
				DMEM[ALUresult+2] = extract_byte(16,SwOp2);
				DMEM[ALUresult+3] = extract_byte(24,SwOp2);
				cout << "    stored :   " ;
				printf( "  %x %x %x %x  ",DMEM[ALUresult+3],DMEM[ALUresult+2],DMEM[ALUresult+1],DMEM[ALUresult]);
				break;
			}
			case(MEM_sh):{
				DMEM[ALUresult] = extract_byte(0,SwOp2);
				DMEM[ALUresult+1] = extract_byte(8,SwOp2);
				cout << "		Stored : " ;
				printf( " %x %x ",DMEM[ALUresult+1],DMEM[ALUresult]);
				break;
			}
			case(MEM_sb):{
				
				DMEM[ALUresult] = extract_byte(0,SwOp2);
				cout << "		Stored : " ;
				printf( "  %x  ",DMEM[ALUresult]);
				break;
			}
		}
	}
}

//writes the results back to register file
void write_back() {
	cout << "\n\nWrite_back:\n";
	cout <<"		rd:" << regdestiny<<endl;
	if(regdestiny){
		if(controls.RFWrite){
			switch(controls.ResultSelect){
				case(From_ALU):{
					X[regdestiny] = ALUresult;
					break;
				}
				case(From_ImmU):{
					X[regdestiny] = Imm_U; 
					break;
				}
				case(From_MEM):{
					X[regdestiny] = Loaded_Data; 
					break;
				}
				case(From_PC):{
					X[regdestiny] = PC + 4; 
					break;
				}
				case(From_AUIPC):{
					X[regdestiny] = PC + Imm_U;
					break;
				}
			}
		}
	}
	
	switch(controls.IsBranch)
	{
		case(NoBranch):{
			PC = PC + 4;
			break;
		}
		case(Branched):{
			PC =  BranchTarget_Addr;
			break;
		}
		case(Branch_From_ALU):{
			PC = ALUresult;
			break;
		}
	}
	cout << "		X[rd]:" << X[regdestiny] << endl;;
}


int read_word(unsigned char *mem, unsigned int address) {
  int *data;
  data =  (int*) (mem + address);
  return *data;
}

void write_word(unsigned char *mem, unsigned int address, unsigned int data) {
  int *data_p;
  data_p = (int*) (mem + address);
  *data_p = data;
}



unsigned int extract_bits(int low, int high){
	unsigned int foo = instruction_register;
	foo = foo << (31 - high);
	foo = foo >> (31+low-high);
	return foo;
}

unsigned int extract_byte(int low, int value){
	unsigned int foo = value;
	foo = foo << (31 - (low + 7));
	foo = foo >> (31 - 7);
	return foo;
}


int sign_extender(int num, int MSB){
	if(num & (1<<MSB)){
		return num - pow(2, MSB+1);
	}
	return num;
}

void viewMEM(int low, int a){
	for(int* p = (int*)&MEM[low], q=a; q>0; p++, q-=4){
		printf("%08x\n", *p);
	}
}
	
