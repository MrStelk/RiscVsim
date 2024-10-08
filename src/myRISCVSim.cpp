
/* 

The project is developed as part of Computer Architecture class
Project Name: Functional Simulator for subset of RISCV Processor

Developer's Name: Karthik N Dasaraju, Pavithran Goud, Kola Sai Datta, Rishik Vardhan Vudem 
*/

/*
commenting for contribution #RISHIKVUDEM
*/

/* myRISCVSim.cpp
   Purpose of this file: implementation file for myRISCVSim
*/

#include <iostream>
#include "myRISCVSim.h"
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <math.h>
#include <vector>
#include <algorithm>

using namespace std;

//Register file
static int X[32];

//memory
static unsigned char MEM[4000000];
map<int,unsigned char > DMEM;

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
#define UtypeL 55
#define UtypeA 23
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
#define BGE 5
#define BLT 4


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

//extracts selected bits from instruction register (machine code)
unsigned int extract_bits(int low, int high);

//extracts selected byte(i.e 0-7/8-15/16-23/24-31) from a value
unsigned int extract_byte(int low, int value);

//extends the sign of a extracted bits
int sign_extender(int num, int MSB);

//prints MEMORY in OUTPUT file
void viewDMEM();

//prints all decoded instructions i instructions file
void print_inst(int opcode,int fun3,int rd,int rs1,int rs2,int imms,int imm,FILE* t);

//compares address and data pair 
bool comp(const pair<int , unsigned char>&a,const pair<int , unsigned char>&b);

unsigned int BranchTarget_Addr;
unsigned int instruction_type;
unsigned int PC = 0;
static unsigned int regdestiny;
static unsigned int SwOp2;
int ALUresult;
int Loaded_Data;
int cycle_no = 0;
int Imm_U;
int Imm_B;
int Imm_J;

FILE *out = fopen("./OUTPUT.txt","w");
FILE *inst= fopen("./instructions.txt","w");

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
//reset all registers and instruction memory content to 0
void reset_proc() {
	for(int i=0; i<32; i++){
		X[i]=0;
	}
	X[2] = 0x7FFFFFDC;
	for(int i=0; i<4000; i++){
		MEM[i] = 0;
	}
	DMEM.clear();
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
  fclose(fp);
}

//writes the data memory in "data_out.mc" file
void write_data_memory() {
	FILE *fp;
	unsigned int i;
	fp = fopen("../output/data_out.mc", "w");
	if(fp == NULL) {
	printf("Error opening dataout.mem file for writing\n");
	return;
	}

	fprintf(fp,"--- REGISTER FILE ---\n\n");

	for(int j = 0;j<32;j++){
		fprintf(fp,"X%d  :  0x%08X \n",j,X[j]);
	}

//sorting the memory by addresses
	vector<pair< int , unsigned char> > vec;
	for(auto& it : DMEM){
	vec.push_back(it);
	}
	sort(vec.begin(),vec.end(),comp);
	if(DMEM.size()){
		fprintf(fp,"\n--- MEMORY ---\n");
		fprintf(fp," ADDRESS     +3 +2 +1 +0");
	}
	else
	fprintf(fp,"\n---NO MEMORY USED ---\n");

	for(auto& m : vec){
		if(!(m.first%4))
		fprintf(fp, "\n0x%08X : %02X %02X %02X %02X ",m.first,DMEM[m.first+3],DMEM[m.first+2],DMEM[m.first+1],DMEM[m.first]);
	}
  fclose(fp);
}

//should be called when instruction is swi_exit
void swi_exit() {
	write_data_memory();
	viewDMEM();
	fclose(out);
	fclose(inst);
  exit(0);
}


int inc;
//reads from the instruction memory and updates the instruction register
void fetch() {
	cout << "Fetch:\n";
	instruction_register = *((unsigned int*)&MEM[PC]);
	cout << "		instruction_register : ";
	printf("%x\n", instruction_register);
	cout << "		PC:" << PC<<endl;
	fprintf(out,"0x%08X:",instruction_register);
	inc++;
	fprintf(inst,"%d:0x%X:0x%08X:",inc,PC,instruction_register);
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
	immJ += (extract_bits(12,19) << 12);
	immJ += (extract_bits(20,20) << 11);	
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
			controls.BranchTarget = Branch_ImmJ;
			operand1 = X[rs1];
			break;
		}
	
		// I-type - load
		case(ItypeL):{
			controls.ALUOp = 0;
			controls.Op2Select = Op2_Imm;	
			controls.ResultSelect = From_MEM;
			controls.RFWrite = Write;
			controls.IsBranch = NoBranch;
			operand1 = X[rs1];
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
			controls.BranchType=-1;
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
			fprintf(out,"EXIT \n");
			fprintf(inst,"EXIT ");
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
    print_inst( opcode, func3, rd, rs1, rs2, immS, imm,out);
	print_inst( opcode, func3, rd, rs1, rs2, immS, imm,inst);
	
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
				if((ALUresult & (1<<31))||(operand1<operand2)){
					controls.IsBranch = NoBranch;		
				}
				break;
			}
			case(BLT):{
				if((!(ALUresult & (1<<31)))||(operand1>=operand2)){

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
			//loads byte
			case(MEM_lb):{
				Loaded_Data = DMEM[ALUresult];
				printf("		loaded :  %02x",Loaded_Data);
				break;
			}
			//loads half word
			case(MEM_lh):{
				Loaded_Data = DMEM[ALUresult+1];
				Loaded_Data = Loaded_Data << 8;
				Loaded_Data = Loaded_Data + DMEM[ALUresult];
				printf("		loaded :  %02x",Loaded_Data);
				break;
			}
			//loads word
			case(MEM_lw):{
				Loaded_Data = DMEM[ALUresult+3];
				Loaded_Data = Loaded_Data << 8;
				Loaded_Data = Loaded_Data + DMEM[ALUresult+2];
				Loaded_Data = Loaded_Data << 8;
				Loaded_Data = Loaded_Data + DMEM[ALUresult+1];
				Loaded_Data = Loaded_Data << 8;
				Loaded_Data = Loaded_Data + DMEM[ALUresult];
				printf("		loaded :  %02x",Loaded_Data);
				break;
			}	
			//stores word	
			case(MEM_sw):{
				DMEM[ALUresult] = extract_byte(0,SwOp2);
				DMEM[ALUresult+1] = extract_byte(8,SwOp2);
				DMEM[ALUresult+2] = extract_byte(16,SwOp2);
				DMEM[ALUresult+3] = extract_byte(24,SwOp2);
				cout << "		stored :   " ;
				printf( "  %02x %02x %02x %02x  ",DMEM[ALUresult+3],DMEM[ALUresult+2],DMEM[ALUresult+1],DMEM[ALUresult]);
				break;
			}
			//stores half word
			case(MEM_sh):{
				DMEM[ALUresult] = extract_byte(0,SwOp2);
				DMEM[ALUresult+1] = extract_byte(8,SwOp2);
				cout << "		Stored : " ;
				printf( " %02x %02x ",DMEM[ALUresult+1],DMEM[ALUresult]);
				break;
			}
			//stores byte
			case(MEM_sb):{
				
				DMEM[ALUresult] = extract_byte(0,SwOp2);
				cout << "		Stored : " ;
				printf( "  %02x  ",DMEM[ALUresult]);
				break;
			}
		}
	}
}

//writes the results back to register file
void write_back() {
	cout << "\n\nWrite_back:\n";
	cout << "		RFWrite :" << controls.RFWrite<<endl;
	
	if(controls.RFWrite){
		if(regdestiny){
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

			cout <<"		rd: X" << regdestiny<<endl;
			cout << "		X[rd]:" << X[regdestiny] << endl;
			fprintf(out,"X%d:0x%08X\n",regdestiny,X[regdestiny]);
		}
		else
		cout << "		rd: X0"<<endl;
	}
	
	viewDMEM();
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
	
	cycle_no++;
	cout << "CYCLE_COMPLETED: "<<cycle_no<<endl;

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

bool comp(const pair<int , unsigned char>&a,const pair<int , unsigned char>&b){
	return a.first > b.first;
}

void viewDMEM(){

	vector<pair< int , unsigned char> > vec;

	for(auto& it : DMEM){
		vec.push_back(it);
	}
	sort(vec.begin(),vec.end(),comp);
	if(DMEM.size())
	fprintf(out,"--- MEMORY ---\n");
	else
	fprintf(out,"-\n");
	for(auto& m : vec){
		if(!(m.first%4))
		fprintf(out, "0x%08X:%02X %02X %02X %02X\n",m.first,DMEM[m.first+3],DMEM[m.first+2],DMEM[m.first+1],DMEM[m.first]);
	}
	fprintf(out,"\n");
}

void print_inst(int opcode,int func3,int rd,int rs1,int rs2,int imms,int imm,FILE *t)
{
	
	switch(opcode){
		// I-type - Arithmetic
		case(ItypeA):
		// R-type
		case(Rtype):{
	
			switch(controls.ALUOp){
			case(Add_op):{
				fprintf(t,"ADD");
				break;
			}
			case(Sub_op):{
				fprintf(t,"SUB");
				break;
			}
			case(Xor_op):{
				fprintf(t,"XOR");
				break;
			}
			case(Or_op):{
				fprintf(t,"OR ");
				break;
			}
			case(And_op):{
				fprintf(t,"AND");
				break;
			}
			case(Sll_op):{
				fprintf(t,"SLL");
				break;
			}
			case(Srl_op):{
				fprintf(t,"SRL");
				break;
			}
			case(Sra_op):{
				fprintf(t,"SRA");
				break;
			}
			case(Slt_op):{
				fprintf(t,"SLT");
				break;
			}
			}
				if(opcode == ItypeA)
				fprintf(t,"i x%d x%d %d",rd,rs1,imm);
				else
				fprintf(t," x%d x%d x%d",rd,rs1,rs2);
			break;
		}	

		// I-type - JALR
		case(ItypeJ):{
			fprintf(t,"JALR x%d x%d %d",rd,rs1,imm);
			break;
		}
	
		// I-type - load
		case(ItypeL):{	
			switch(func3){
				case(0):{
					fprintf(t,"LB");
					break;
				}
				case(1):{
					fprintf(t,"LH");
					break;
				}
				case(2):{
					fprintf(t,"LW");
					break;
				}
			}
			fprintf(t," x%d %d(x%d)",rd,imm,rs1);
			break;
		}	
	
		
		
		// S-type
		case(Stype):{
			
			switch(func3){
				case(0):{
					fprintf(t,"SB");
					break;
				}
				case(1):{
					fprintf(t,"SH");
					break;
				}
				case(2):{
					fprintf(t,"SW");
					break;
				}
			}
			fprintf(t," x%d %d(x%d)",rs2,imms,rs1);
			break;
		}
	
		// B-type
		case(Btype):{
			switch(controls.BranchType){
			case(BEQ):{
				fprintf(t,"BEQ");
				break;
			}
			case(BNE):{
				fprintf(t,"BNE");
				break;
			}
			case(BGE):{
				fprintf(t,"BGE");
				break;
			}
			case(BLT):{
				fprintf(t,"BLT");
				break;
			}
			}
			fprintf(t," x%d x%d %d",rs1,rs2,Imm_B);
			break;
		}
		
		// J-type
		case(Jtype):{
			fprintf(t,"JAL x%d %d",rd,Imm_J);
			break;
		}
	
		// U-type - auipc
		case(UtypeA):{
			fprintf(t,"AUIPC x%d 0x%x",rd ,Imm_U>>12);
			break;
		}

		// U-type - lui
		case(UtypeL):{
			fprintf(t,"LUI x%d 0x%x",rd,Imm_U>>12);
			break;
		}
		
	}
	fprintf(t,"\n");
}	
