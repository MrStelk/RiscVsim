/* 

The project is developed as part of Computer Architecture class
Project Name: Functional Simulator for subset of RISCV Processor

Developer's Name: Karthik N Dasaraju, Pavithran Goud, Kola Sai Datta, Rishik Vardhan Vudem 



   myRISCVSim.cpp
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
#define Op2_ImmU 3


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

//for hazard
#define hz_1 1
#define hz_2 2
#define hz_3 3
#define hz_b 4

#define f_st 2
#define f_stall 1

//for exit
#define _EXIT 1
#define NO_EXIT 0

#define taken 1
#define not_taken 0

//for cache mapping
#define direct 0
#define fully_associative 1
#define set_associative 2

//for replacement policy
#define LRU 0
#define FIFO 1
#define random 2
#define LFU 3

#define MISS_PENALITY 20

int ins_hits=0;
int ins_misses=0;
int ins_cold=0;
int ins_conflict=0;
int ins_capacity=0;
int ins_accesses=0;
float ins_hitrate=0;
float ins_missrate=0;

int memory_stalls=0;//////////////////////////////
int extra_cycles=0;///////////////////////////////
float CPI_perfect=0;

int data_hits=0;
int data_misses=0;
int data_cold=0;
int data_conflict=0;
int data_capacity=0;
int data_writemisses=0;
int data_accesses=0;
float data_hitrate=0;
float data_missrate=0;


int data_cachesize = pow(2, 10);
int data_blocksize = pow(2, 4);
int data_mapping = 0;
int data_ways = pow(2, 2);
int data_replacepolicy = 1;
int data_cachelines = data_cachesize/data_blocksize;

void datacache(int opr, int addr, int type);

char input_byte=0;
short int input_halfword=0;
int input_word=0;
char output_byte=0;
short output_halfword=0;
int output_word=0;

typedef struct{
	int RF;
	bool update;
	bool valid;
	unsigned int tag;
	unsigned char* words;
}data_line;
data_line* data_cache;

int ins_cachesize = pow(2, 4);//words
int ins_blocksize = pow(2, 2);//words
int ins_mapping = 2;
int ins_ways = pow(2, 2);
int ins_replacepolicy = 1;
int ins_cachelines = ins_cachesize/ins_blocksize;

int inscache(unsigned int addr);
void free_malloc();
void write_to_DMEM();

typedef struct{
	int RF;
	bool update;
	bool valid;
	unsigned int tag;
	int* words;
}ins_line;

ins_line* ins_cache;

int forward_knob;
int print_regfile; 
int print_pprg;
int spec_instruction;

// All control signals.
typedef struct{
	unsigned int ALUOp;
	unsigned int IsBranch;
	unsigned int MemOp;
	unsigned int Op2Select;
	unsigned int ResultSelect;
	unsigned int BranchTarget;
	unsigned int RFWrite;

	unsigned int BranchType;
	int forward_type;
	int HAZARD_b;

}control;

struct {
	int null;
	unsigned int pc;
	unsigned int instruction;
} F_D_reg,hazard_dup;

struct {
	int null;
	unsigned int pc;
	unsigned int instruction;
	int operand1;
	int operand2;
	int op2;
	unsigned int BranchTarget_Addr; 
	control controls;
}D_E_reg;

struct{
	int null;
	unsigned int pc;
	unsigned int instruction;
	int ALUresult;
	int op2;
	int bp_result;
	control controls;

	unsigned int BranchTarget_Addr; 
}E_M_reg;

struct {
	int null;
	unsigned int pc;
	unsigned int instruction;
	int ALUresult;
	int ld_result;
	control controls;
}M_W_reg;

struct {
	int rd1;
	int rd2;
	int rd3;
}HZ_data;

//extracts selected bits from instruction register (machine code)
unsigned int extract_bits(int low, int high);

//extracts selected byte(i.e 0-7/8-15/16-23/24-31) from a value
unsigned int extract_byte(int low, int value);

//extends the sign of a extracted bits
int sign_extender(int num, int MSB);

void viewDMEM();

void pipe_reg();

//prints all decoded instructions i instructions file
void print_inst(unsigned int _pc , unsigned int instruction ,FILE* );

//compares address and data pair 
bool comp(const pair<int , unsigned char>&a,const pair<int , unsigned char>&b);

void reg_file();

unsigned int BranchTarget_Addr;
unsigned int instruction_type;
unsigned int PC = 0;
static unsigned int regdestiny;
static unsigned int SwOp2;
int ALUresult;
int Loaded_Data;
char output__byte=0;
short output__halfword=0;
int output__word=0;
int cycle_no = 0;
int Imm_U;
int Imm_B;
int Imm_J;
int EXIT_signl;
unsigned int EXIT_pc;
int HAZARD;
int HAZARD_b;
int Hazard_type;

int in_ex ;
int Data_transfers;
int ALU_inst;
int control_inst;
int stalls;
int data_hz;
int control_hz;
int branch_misp;
int stall_d;
int stall_c;

int branch_prediction;

FILE *out = fopen("./OUTPUT.txt","w");
FILE *inst = fopen("./inst_pipe.txt","w");
FILE *rfile = fopen("../output/reg_file.txt", "w");
FILE *brnch = fopen("./branch.txt","w");
FILE *inpt = fopen("./knobs.txt","r");
FILE *out1 = fopen("./MEM.txt","w");
FILE *pfile = fopen("../output/pipeline_reg.txt", "w");

void run_riscvsim() {
  while(1) {
  	cout << "\n\n---------New cycle---------\n";
		write_back(); 
		mem();
		execute();
		decode();
		fetch();
		viewDMEM();
   cycle_no++;
	cout << "CYCLE_COMPLETED: "<<cycle_no<<endl;
	fprintf(out,"%d\n",cycle_no);
	if(print_regfile){
		reg_file();
	 }
	if(print_pprg){
		pipe_reg();
	}
  }
}

// it is used to set the reset values
//reset all registers and instruction memory content to 0
void reset_proc() {
	data_cache = (data_line*)malloc(sizeof(data_line)*data_cachelines);
	for(int i=0; i<data_cachelines; i++)
	{
		data_cache[i].words = (unsigned char*)malloc(sizeof(unsigned char)*data_blocksize);
	}
	ins_cache = (ins_line*)malloc(sizeof(ins_line)*ins_cachelines);
	for(int i=0; i<ins_cachelines; i++)
	{
		ins_cache[i].words = (int*)malloc(sizeof(int)*ins_blocksize);
	}
	for(int i=0; i<32; i++){
		X[i]=0;
	}
	X[2] = 0x7FFFFFDC;
	for(int i=0; i<4000; i++){
		MEM[i] = 0;
	}
	DMEM.clear();

	E_M_reg.bp_result=1;

	F_D_reg.null=1;
	D_E_reg.null=1;
	E_M_reg.null=1;
	M_W_reg.null=1;

	HAZARD = 0;
	HAZARD_b = 0;
	HZ_data.rd1 = 32;
	HZ_data.rd2 = 32;
	HZ_data.rd3 = 32;
	EXIT_signl = NO_EXIT;
	EXIT_pc = -1;

	Data_transfers=0;
	ALU_inst=0;
	control_inst=0;
	stalls=0;
	data_hz=0;
	control_hz=0;
	branch_misp=0;
	stall_d=0;
	stall_c=0;
	branch_prediction = 0;

	for(int i=0; i<ins_cachelines; i++)
	{
		ins_cache[i].update = 0;
		ins_cache[i].valid = 0;
		ins_cache[i].tag = 0;
	}

	for(int i=0; i<data_cachelines; i++)
	{
		data_cache[i].update = 0;
		data_cache[i].valid = 0;
		data_cache[i].tag = 0;
	}

	if(ins_replacepolicy == 0)
	{
		for(int i=0; i<ins_cachelines; i++)
		{
			ins_cache[i].RF = i;
		}
	}
	else if(ins_replacepolicy == 1)
	{
		for(int i=0; i<ins_cachelines; i++)
		{
			ins_cache[i].RF = 0;
		}
		ins_cache[0].RF = 1;
	}
	else if(ins_replacepolicy == 2)
	{
		for(int i=0; i<ins_cachelines; i++)
		{
			ins_cache[i].RF = 0;
		}
	}

	if(data_replacepolicy == 0)
	{
		for(int i=0; i<data_cachelines; i++)
		{
			data_cache[i].RF = i;
		}
	}
	else if(data_replacepolicy == 1)
	{
		for(int i=0; i<data_cachelines; i++)
		{
			data_cache[i].RF = 0;
		}
		data_cache[0].RF = 1;
	}
	else if(data_replacepolicy == 2)
	{
		for(int i=0; i<data_cachelines; i++)
		{
			data_cache[i].RF = 0;
		}
	}


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
	print_inst(address,instruction,inst);
    write_word(&MEM[0], address, instruction);
  }

  fscanf(inpt,"%d %d %d %d",&forward_knob,&print_regfile,&print_pprg,&spec_instruction);
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
  FILE* ofile;
  ofile = fopen("../output/Output_File.txt", "w");
  if( ofile == NULL) {
	printf("Error opening dataout.mem file for writing\n");
	return;
	}
	stalls = stall_c+stall_d;
	ALU_inst = in_ex + ALU_inst;

	fprintf(ofile,"No.of Cycles = %d\n",cycle_no);
	fprintf(ofile,"Instructions executed = %d\n",in_ex);
	fprintf(ofile,"No.of Cycles Per Insruction = %d\n",cycle_no/in_ex);
	fprintf(ofile,"No.of Data Transfers = %d\n",Data_transfers);
	fprintf(ofile,"No.of ALU Instructions = %d\n",ALU_inst);
	fprintf(ofile,"No.of Control Instructions = %d\n",control_inst);
	fprintf(ofile,"No.of stalls = %d\n",stalls);
	fprintf(ofile,"No.of data hazards = %d\n",data_hz);
	fprintf(ofile,"No.of control hazards = %d\n",control_hz);
	fprintf(ofile,"No.of Branch mispredictions = %d\n",branch_misp);
	fprintf(ofile,"No.of stalls due to data hazards  = %d\n",stall_d);
	fprintf(ofile,"No.of stalls due to control hazards  = %d\n\n",stall_c);

	ins_misses = ins_cold + ins_conflict + ins_capacity;
	data_misses = data_cold + data_conflict + data_capacity;
	ins_hitrate = ((float)ins_hits/((float)ins_hits + (float)ins_misses))*100;
	ins_missrate = 100 - ins_hitrate;
	data_hitrate = ((float)data_hits/((float)data_hits + (float)data_misses))*100;
	data_missrate = 100 - data_hitrate;

	memory_stalls = (ins_misses + data_misses)*MISS_PENALITY;
	extra_cycles = memory_stalls;
	CPI_perfect = (float)(cycle_no+extra_cycles)/(float)in_ex;

	fprintf(ofile,"ins_accesses = %d\n", ins_accesses);
	fprintf(ofile,"ins_hits = %d\n", ins_hits);
	fprintf(ofile,"ins_misses = %d\n", ins_misses);
	fprintf(ofile,"ins_cold = %d\n", ins_cold);
	fprintf(ofile,"ins_conflict = %d\n", ins_conflict);
	fprintf(ofile,"ins_capacity = %d\n", ins_capacity);
	fprintf(ofile,"ins_hitrate = %f\n", ins_hitrate);
	fprintf(ofile,"ins_missrate = %f\n\n", ins_missrate);

	fprintf(ofile,"data_accesses = %d\n", data_accesses);
	fprintf(ofile,"data_hits = %d\n", data_hits);
	fprintf(ofile,"data_misses = %d\n", data_misses);
	fprintf(ofile,"data_cold = %d\n", data_cold);
	fprintf(ofile,"data_conflict = %d\n", data_conflict);
	fprintf(ofile,"data_capacity = %d\n", data_capacity);
	fprintf(ofile,"data_writemisses = %d\n\n", data_writemisses);
	fprintf(ofile,"data_hitrate = %f\n", data_hitrate);
	fprintf(ofile,"data_missrate = %f\n\n", data_missrate);

	fprintf(ofile,"memory_stalls = %d\n", memory_stalls);
	fprintf(ofile,"extra_cycles = %d\n", extra_cycles);
	fprintf(ofile,"CPI_perfect = %f\n", CPI_perfect);

	fclose(ofile);
}

//should be called when instruction is swi_exit
void swi_exit() {
	write_to_DMEM();
	write_data_memory();
	fclose(out);
	fclose(rfile);
	fclose(inpt);
	fclose(brnch);
	fclose(inst);
	fclose(out1);
	free_malloc();
  exit(0);
}

int inc;
//reads from the instruction memory and updates the instruction register
void fetch() {
	
	cout << "Fetch:\n";
	if(EXIT_signl)
	{	cout << "             []\n";
		fprintf(out,"FI:\n");
		F_D_reg.null = 1;
		
	}
	else{
	F_D_reg.null = 0;
	F_D_reg.instruction = inscache(PC);
	instruction_register = F_D_reg.instruction;
	F_D_reg.pc = PC; 
	cout << "		instruction_register : ";
	
	printf("0x%08X\n		PC:0x%X\n", instruction_register,PC);
	fprintf(out,"FI:%d\n",(F_D_reg.pc/4)+1);
	}

	inc++;
	

	if(E_M_reg.bp_result == false)
	{
		branch_misp++;
		stall_c=stall_c+2;
		if(branch_prediction == not_taken)
			PC = E_M_reg.BranchTarget_Addr;
		else
			PC = E_M_reg.pc + 4;

		branch_prediction = !branch_prediction;
		E_M_reg.bp_result = true;
		F_D_reg.null  = 1;
		HZ_data.rd1 = 32;
		HZ_data.rd2 = 32;
		HZ_data.rd3 = 32;
		D_E_reg.null  = 1;
		if(EXIT_signl)
		EXIT_signl = NO_EXIT;
		HAZARD = 0;
		return;
	}
	else
	{

		if(!E_M_reg.null){

		instruction_register = E_M_reg.instruction;
		int dummy = extract_bits(0, 6);
		instruction_register = D_E_reg.instruction;
		int dummy1 = extract_bits(0, 6);
		if(dummy == Jtype)
		{
			if(dummy1 != Btype)
			{
				control_hz++;
				control_inst++;
			}
			stall_c = stall_c+2;
			PC = E_M_reg.BranchTarget_Addr;
			E_M_reg.bp_result = true;
			F_D_reg.null  = 1;
			HZ_data.rd1 = 32;
			HZ_data.rd2 = 32;
			HZ_data.rd3 = 32;
				
			D_E_reg.null  = 1;
			if(EXIT_signl)
			EXIT_signl = NO_EXIT;
				
			HAZARD = 0;
			return;
		}
		else if(dummy == ItypeJ)
		{
			if(dummy1 != Btype)
			{
				control_hz++;
				control_inst++;
			}
			stall_c = stall_c+2;
			PC = E_M_reg.ALUresult;
			E_M_reg.bp_result = true;
			F_D_reg.null  = 1;
			HZ_data.rd1 = 32;
			HZ_data.rd2 = 32;
			HZ_data.rd3 = 32;
			D_E_reg.null  = 1;
			if(EXIT_signl)
			EXIT_signl = NO_EXIT;
			HAZARD = 0;
			return;
		}
		}
				if(HAZARD)
				{
					F_D_reg = hazard_dup;
					return;
				}
			
			instruction_register = F_D_reg.instruction;
			int immB = (extract_bits(31,31) << 12);
			immB += (extract_bits(7,7) << 11);
			immB += (extract_bits(25,30) << 5);
			immB += (extract_bits(8,11) << 1);
			immB = sign_extender(immB, 12);
			
			int opcode = extract_bits(0, 6);
			if(opcode == Btype)
			{
				control_hz++;
				control_inst++;
				if(branch_prediction == not_taken)
					PC = PC + 4;
				else
					PC =  PC + immB;
			}
			else
			{
				
				PC = PC + 4; 
			}
		
	}
}

//reads the instruction register, reads operand1, operand2 fromo register file, decides the operation to be performed in execute stage
void decode() {
	
	cout << "\n\nDecode:\n";

	if(F_D_reg.null)
	{
		cout << "             []\n";
		fprintf(out,"DE:\n");
		D_E_reg.null = 1;
		return;
	}
	unsigned int func3, rs1, rs2, rd, opcode;	
	instruction_register = F_D_reg.instruction;
	D_E_reg.instruction = F_D_reg.instruction;
	D_E_reg.pc = F_D_reg.pc;
	D_E_reg.null = 0;

	D_E_reg.controls.HAZARD_b = 0;
	cout << "		instruction_register : ";
	
	printf("0x%08X\n		PC:0x%X\n", instruction_register,D_E_reg.pc);
	fprintf(out,"DE:%d",(D_E_reg.pc/4)+1);
	if(HAZARD){
		
			HZ_data.rd3 = HZ_data.rd2;
			HZ_data.rd2 = HZ_data.rd1;
			HZ_data.rd1 = 32 ;
			
       if(!forward_knob)
		{if(--Hazard_type)
		{
			stall_d++;
			D_E_reg.null = 1;
			fprintf(out,"\n");
			return;
		}
		else{
				data_hz++;
				stall_d++;
				HAZARD = 0;
		}
		}
		else{
			stall_d++;
		}
	}
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

	D_E_reg.operand2 = X[rs2];

	switch(opcode){
		// R-type
		case(Rtype):{
		D_E_reg.controls.Op2Select = Op2_RF;
			D_E_reg.controls.MemOp = NoMEMOp;
			D_E_reg.controls.IsBranch = NoBranch;
			D_E_reg.controls.ResultSelect = From_ALU;	
			D_E_reg.controls.RFWrite = Write;		
	
			func3 = extract_bits(12,14);
			D_E_reg.controls.ALUOp = func3;
	
			unsigned int func7 = extract_bits(25,31);
			if(func7==32){
				D_E_reg.controls.ALUOp += func7;	
			}
			
			D_E_reg.operand1 = X[rs1];	
			regdestiny = rd;
			break;
		}	

		// I-type - JALR
		case(ItypeJ):{
			D_E_reg.controls.ALUOp = 0;
			D_E_reg.controls.Op2Select = Op2_Imm;
			D_E_reg.controls.ResultSelect = From_PC;
			D_E_reg.controls.MemOp = NoMEMOp;
			D_E_reg.controls.RFWrite = Write;
			D_E_reg.controls.IsBranch = Branch_From_ALU;
			D_E_reg.controls.BranchTarget = Branch_ImmJ;
			D_E_reg.operand1 = X[rs1];
			D_E_reg.controls.HAZARD_b = 1;
			rs2 = 33;
			break;
		}
	
		// I-type - load
		case(ItypeL):{
			D_E_reg.controls.ALUOp = 0;
			D_E_reg.controls.Op2Select = Op2_Imm;
			D_E_reg.controls.ResultSelect = From_MEM;
			D_E_reg.controls.RFWrite = Write;
			D_E_reg.controls.IsBranch = NoBranch;
			D_E_reg.operand1 = X[rs1];
			rs2 = 33;
			func3 = extract_bits(12,14);	
			switch(func3){
				case(0):{
					D_E_reg.controls.MemOp = MEM_lb;
					break;
				}
				case(1):{
					D_E_reg.controls.MemOp = MEM_lh;
					break;
				}
				case(2):{
					D_E_reg.controls.MemOp = MEM_lw;
					break;
				}
			}
			break;
		}	
	
		// I-type - Arithmetic
		case(ItypeA):{
			D_E_reg.controls.Op2Select = Op2_Imm;
			D_E_reg.controls.MemOp = NoMEMOp;
			D_E_reg.controls.ResultSelect = From_ALU;
			D_E_reg.controls.RFWrite = Write;
			D_E_reg.controls.IsBranch = NoBranch;
			
			func3 = extract_bits(12,14);
			D_E_reg.controls.ALUOp = func3;
			cout << "		Fun3:" << func3 << endl;
			D_E_reg.operand1 = X[rs1];
			rs2  = 33;
			break;
		}
		
		// S-type
		case(Stype):{
			D_E_reg.controls.Op2Select = Op2_ImmS;
			D_E_reg.controls.ALUOp = 0;
			D_E_reg.controls.RFWrite = NoWrite;
			D_E_reg.controls.IsBranch = NoBranch;
			rd = 32;
			func3 = extract_bits(12,14);	
			switch(func3){
				case(0):{
					D_E_reg.controls.MemOp = MEM_sb;
					break;
				}
				case(1):{
					D_E_reg.controls.MemOp = MEM_sh;
					break;
				}
				case(2):{
					D_E_reg.controls.MemOp = MEM_sw;
					break;
				}
			}

			D_E_reg.operand1 = X[rs1];
			D_E_reg.op2= X[rs2];
			break;
		}
	
		// B-type
		case(Btype):{
			D_E_reg.operand1=X[rs1];
			D_E_reg.operand2=X[rs2];
			func3 = extract_bits(12,14);
			D_E_reg.controls.ALUOp=32;
			D_E_reg.controls.Op2Select= Op2_RF;
			D_E_reg.controls.RFWrite= NoWrite;
			D_E_reg.controls.IsBranch= Branched;
			D_E_reg.controls.BranchTarget= Branch_ImmB;
			D_E_reg.controls.BranchType=func3;
        		D_E_reg.controls.MemOp=NoMEMOp;
			rd = 32;
			D_E_reg.controls.HAZARD_b = 1;

		
			break;
		}
		
		// J-type
		case(Jtype):{
			D_E_reg.controls.RFWrite= Write;
			D_E_reg.controls.ResultSelect= From_PC;
			D_E_reg.controls.BranchTarget= Branch_ImmJ;
			D_E_reg.controls.IsBranch= Branched;
			D_E_reg.controls.MemOp=NoMEMOp;
			D_E_reg.controls.BranchType=-1;
			D_E_reg.controls.HAZARD_b = 1;
			rs1 = 34;
			rs2 = 33;
			
			break;
		}
	
		// U-type - auipc
		case(UtypeA):{
			D_E_reg.controls.RFWrite= Write;
			D_E_reg.controls.ResultSelect= From_AUIPC;
			D_E_reg.controls.ALUOp=0;
			D_E_reg.controls.IsBranch=NoBranch;
			D_E_reg.controls.MemOp=NoMEMOp;
			D_E_reg.controls.Op2Select = Op2_ImmU;
			D_E_reg.operand1=0;
			rs1 = 34;
			break;
		}

		// U-type - lui
		case(UtypeL):{
			D_E_reg.controls.RFWrite= Write;
			D_E_reg.controls.ALUOp=0;
			D_E_reg.controls.ResultSelect= From_ImmU;
			D_E_reg.controls.MemOp= NoMEMOp;
			D_E_reg.controls.IsBranch=NoBranch;
			D_E_reg.controls.Op2Select = Op2_ImmU;
			D_E_reg.operand1=0;
			rs1 = 34;
			break;
		}
		
		// EXIT
		case(EXIT):{
			EXIT_signl = _EXIT;
			EXIT_pc = D_E_reg.pc;
			rd = 32;
			
		}
	}

	// Selects OP2 for ALU.
	
		int H_val=0, H_val2 = 0;
		if(rs1 == HZ_data.rd1 || rs2 == HZ_data.rd1)
		{
			H_val =  (rs1 == HZ_data.rd1) ? 1 : 2 ;
			HAZARD = 1;
			Hazard_type = hz_3;
			D_E_reg.null = 1;
			//fprintf(out,":1:1");
		
		}
		else if(rs1 == HZ_data.rd2 || rs2 == HZ_data.rd2)
		{
			H_val =  (rs1 == HZ_data.rd2) ? 1 : 2 ;
			HAZARD = 1;
			Hazard_type = hz_2;
			D_E_reg.null = 1;
			//fprintf(out,":1:2");
		
		}
		else if(rs1 == HZ_data.rd3 || rs2 == HZ_data.rd3)
		{
			H_val =  (rs1 == HZ_data.rd3) ? 1 : 2 ;
			HAZARD = 1;
			Hazard_type = hz_1;
			D_E_reg.null = 1;
		//	fprintf(out,":1:3");
		}
		
		D_E_reg.controls.forward_type=0;

		if(HAZARD){
         if(forward_knob)
			{	if(Hazard_type == hz_3)
				{	
					if(rs1 == HZ_data.rd2 || rs2 == HZ_data.rd2){
						data_hz++;
						H_val2 =  (rs1 == HZ_data.rd2) ? 1 : 2 ;
								switch(M_W_reg.controls.ResultSelect){
						case(From_ALU):{
							if(H_val2==1){
							D_E_reg.operand1=M_W_reg.ALUresult;
							}
							else{
								D_E_reg.operand2=M_W_reg.ALUresult;
							}
							break;
						}
						case(From_ImmU):{
							if(H_val2==1){
							D_E_reg.operand1=M_W_reg.ALUresult;
							}
							else{
								D_E_reg.operand2=M_W_reg.ALUresult;
							}
							break;
						}
						case(From_PC):{
							if(H_val2==1){
							D_E_reg.operand1=M_W_reg.pc + 4;
							}
							else{
								D_E_reg.operand2=M_W_reg.pc + 4;
							}
								break;
						}
						case(From_MEM):{
							if(H_val2==1){
							D_E_reg.operand1=M_W_reg.ld_result;
							}
							else{
								D_E_reg.operand2=M_W_reg.ld_result;
							}
							break;
						}

						case(From_AUIPC):{
							if(H_val2==1){
							D_E_reg.operand1=M_W_reg.pc + M_W_reg.ALUresult;
							}
							else{
								D_E_reg.operand2=M_W_reg.pc + M_W_reg.ALUresult;
							}
							break;
							}
						}
					}
				}
				
				if(rs1==rs2)
				{
					H_val=3;
				}
				data_hz++;
				HAZARD=0;
				D_E_reg.null=0;
                switch(Hazard_type) {     
					case hz_3:
						{
						switch(E_M_reg.controls.ResultSelect){
						case(From_ALU):{
						//	fprintf(out,":1");
							if(H_val==1){
							D_E_reg.operand1=E_M_reg.ALUresult;
						}
						else{
							D_E_reg.operand2=E_M_reg.ALUresult;
						}
						break;
						}
						case(From_ImmU):{
						//	fprintf(out,":1");
							if(H_val==1){
							D_E_reg.operand1=E_M_reg.ALUresult;
						}
						else{
							D_E_reg.operand2=E_M_reg.ALUresult;
						}
						break;
						}
						case(From_PC):{
						//fprintf(out,":1");
							if(H_val==1){
							D_E_reg.operand1=E_M_reg.pc + 4;
						}
						else{
							D_E_reg.operand2=E_M_reg.pc + 4;
						}
							break;
						}
						case(From_MEM):{
							if(opcode==Stype)
							{
								if(H_val==1)
								{
									D_E_reg.controls.forward_type=f_stall;
									D_E_reg.null=1;
									data_hz--;
									HAZARD=1;
								}
								else{
										D_E_reg.controls.forward_type=f_st;
								}
							}
							else{
								D_E_reg.controls.forward_type=f_stall;
								D_E_reg.null=1;
								data_hz--;
								HAZARD=1;
							}
							break;
						}

						case(From_AUIPC):{
							//fprintf(out,":1");
							if(H_val==1){
							D_E_reg.operand1=E_M_reg.pc + E_M_reg.ALUresult;
							}
							else{
								D_E_reg.operand2=E_M_reg.pc + E_M_reg.ALUresult;
							}
							break;
						}
						}
						break;
						}
					case hz_2:
						{
						switch(M_W_reg.controls.ResultSelect){
						case(From_ALU):{
							//fprintf(out,":2");
							if(H_val==1){
							D_E_reg.operand1=M_W_reg.ALUresult;
							}
							else{
								D_E_reg.operand2=M_W_reg.ALUresult;
							}
							break;
						}
						case(From_ImmU):{
						//	fprintf(out,":2");
							if(H_val==1){
							D_E_reg.operand1=M_W_reg.ALUresult;
							}
							else{
								D_E_reg.operand2=M_W_reg.ALUresult;
							}
							break;
						}
						case(From_PC):{
							//fprintf(out,":2");
							if(H_val==1){
							D_E_reg.operand1=M_W_reg.pc + 4;
							}
							else{
								D_E_reg.operand2=M_W_reg.pc + 4;
							}
								break;
						}
						case(From_MEM):{
							//fprintf(out,":2");
							if(H_val==1){
							D_E_reg.operand1=M_W_reg.ld_result;
							}
							else{
								D_E_reg.operand2=M_W_reg.ld_result;
							}
							break;
						}

						case(From_AUIPC):{
							//fprintf(out,":2");
							if(H_val==1){
							D_E_reg.operand1=M_W_reg.pc + M_W_reg.ALUresult;
							}
							else{
								D_E_reg.operand2=M_W_reg.pc + M_W_reg.ALUresult;
							}
							break;
							}
						}
						break;
						}
					case hz_1:{
						Hazard_type=0;
						break;
					}
							 
				}
				Hazard_type=0;
				if(H_val==3)
				{//fprintf(out,":3");
					D_E_reg.operand1=D_E_reg.operand2;
				}
			}
			D_E_reg.op2 = D_E_reg.operand2;
		}
			
			// Selects OP2 for ALU.
		switch(D_E_reg.controls.Op2Select){
		case(Op2_Imm):{
			D_E_reg.operand2 = imm;
			rs2 = 33;
			break;
		}
		case(Op2_ImmS):{
			D_E_reg.operand2 = immS;
			break;
		}
		case(Op2_ImmU):{
			D_E_reg.operand2 = immU;
			rs2 = 33;
			break;
		}
	}
		if(HAZARD)
		{
			hazard_dup = F_D_reg;
		}
fprintf(out,"\n");
		if(D_E_reg.null)
		 return;
	
	HZ_data.rd3 = HZ_data.rd2;
	HZ_data.rd2 = HZ_data.rd1;
	HZ_data.rd1 = rd ;

	if(HZ_data.rd1 == 0)
		HZ_data.rd1 = 32;

	if(D_E_reg.controls.BranchTarget == Branch_ImmJ){
		D_E_reg.BranchTarget_Addr = Imm_J + D_E_reg.pc;	
	}
	else if (D_E_reg.controls.BranchTarget == Branch_ImmB){
		D_E_reg.BranchTarget_Addr = Imm_B + D_E_reg.pc;
	}
	
	cout << "		operand1:" << D_E_reg.operand1<<endl;
	cout << "		operand2:" << D_E_reg.operand2 << endl;
}

//executes the ALU operation based on ALUop
void execute(){
	cout << "\n\nExecute:\n";

	if(D_E_reg.null)
	{
		cout << "             []\n";
		fprintf(out,"EX:\n");
		E_M_reg.controls.IsBranch=NoBranch;
		E_M_reg.null = 1;
		return;
	}
	

	int add,sub,_xor,_or,_and,sll,sra,slt,sltu;
	unsigned int srl;

	instruction_register = D_E_reg.instruction;
	E_M_reg.instruction = D_E_reg.instruction;
	E_M_reg.pc = D_E_reg.pc;
	E_M_reg.op2 = D_E_reg.op2;

	E_M_reg.BranchTarget_Addr  = D_E_reg.BranchTarget_Addr;

	cout << "		instruction_register : ";
	printf("0x%08X\n		PC:0x%X\n", instruction_register,E_M_reg.pc);
	fprintf(out,"EX:%d",(E_M_reg.pc/4)+1);
	if(E_M_reg.pc == EXIT_pc){
		fprintf(out,"\n");
		E_M_reg.null = 0;
		return;
	}
	add = D_E_reg.operand1 + D_E_reg.operand2;
	sub = D_E_reg.operand1 - D_E_reg.operand2;
	_xor = D_E_reg.operand1 ^ D_E_reg.operand2;
	_or = D_E_reg.operand1 | D_E_reg.operand2;
	_and = D_E_reg.operand1 & D_E_reg.operand2;
	sll = D_E_reg.operand1 << D_E_reg.operand2;
	srl = (unsigned int)D_E_reg.operand1 >> D_E_reg.operand2;
	sra = D_E_reg.operand1 >> D_E_reg.operand2;
	slt = (D_E_reg.operand1 < D_E_reg.operand2)? 1 : 0 ;
	
	cout << "		ALUop:" << D_E_reg.controls.ALUOp << endl;
	switch(D_E_reg.controls.ALUOp){
		case(Add_op):{
			E_M_reg.ALUresult = add;
			break;
		}
		case(Sub_op):{
			E_M_reg.ALUresult = sub;
			break;
		}
		case(Xor_op):{
			E_M_reg.ALUresult = _xor;
			break;
		}
		case(Or_op):{
			E_M_reg.ALUresult = _or;
			break;
		}
		case(And_op):{
			E_M_reg.ALUresult = _and;
			break;
		}
		case(Sll_op):{
			E_M_reg.ALUresult = sll;
			break;
		}
		case(Srl_op):{
			E_M_reg.ALUresult = srl;
			break;
		}
		case(Sra_op):{
			E_M_reg.ALUresult = sra;
			break;
		}
		case(Slt_op):{
			E_M_reg.ALUresult = slt;
			break;
		}
	}
	cout << "		ALUresult:" << E_M_reg.ALUresult<<endl;
	
	if(D_E_reg.controls.IsBranch == Branched){
		switch(D_E_reg.controls.BranchType){
			case(BEQ):{
				if(E_M_reg.ALUresult){
					D_E_reg.controls.IsBranch = NoBranch;
					
				}
				break;
			}
			case(BNE):{
				if(!E_M_reg.ALUresult){
					D_E_reg.controls.IsBranch = NoBranch;
				}
				break;
			}
			case(BGE):{
				if((E_M_reg.ALUresult & (1<<31))||(D_E_reg.operand1<D_E_reg.operand2)){
					D_E_reg.controls.IsBranch = NoBranch;		
				}
				break;
			}
			case(BLT):{
				if((!(E_M_reg.ALUresult & (1<<31)))||(D_E_reg.operand1>=D_E_reg.operand2)){

					D_E_reg.controls.IsBranch = NoBranch;
				}
				break;
			}
		}
		instruction_register = D_E_reg.instruction;
		int dummy = extract_bits(0, 6);
		
		//if(dummy == ItypeJ)
		//fprintf(out,":0");

		if(dummy == Jtype){
			//fprintf(out,":0");
			E_M_reg.bp_result = true;
		}
		else{
		if(branch_prediction == not_taken && D_E_reg.controls.IsBranch == NoBranch)
		{
			fprintf(brnch,"%d:%d:0:0\n",cycle_no+1,(E_M_reg.pc/4)+1);
			E_M_reg.bp_result = true;
		}
		else if(branch_prediction == taken && D_E_reg.controls.IsBranch == Branched)
		{
			fprintf(brnch,"%d:%d:1:1\n",cycle_no+1,(E_M_reg.pc/4)+1);
			E_M_reg.bp_result = true;
		}
		else if(branch_prediction == not_taken && D_E_reg.controls.IsBranch == Branched)
		{
			fprintf(brnch,"%d:%d:0:1\n",cycle_no+1,(E_M_reg.pc/4)+1);
			//fprintf(out,":0");
			E_M_reg.bp_result = false;
		}
		else if(branch_prediction == taken && D_E_reg.controls.IsBranch == NoBranch)
		{
			fprintf(brnch,"%d:%d:1:0\n",cycle_no+1,(E_M_reg.pc/4)+1);
			//fprintf(out,":0");
			E_M_reg.bp_result = false;
		}
		}
		
	}
	
	E_M_reg.controls = D_E_reg.controls;
		E_M_reg.null = 0 ;
	if(forward_knob)
	{
       if(E_M_reg.controls.forward_type==f_st)
	   {
		//fprintf(out,":1:1:1");
          E_M_reg.op2=M_W_reg.ld_result;
	   }

	}

	fprintf(out,"\n");
}

//perform the memory operation
void mem() {
	
	cout << "\n\nMemory:\n";
	if(E_M_reg.null)
	{	cout << "             []\n";
		fprintf(out,"MA:\n");
		M_W_reg.controls.HAZARD_b = 0;
		M_W_reg.controls.IsBranch=NoBranch;
		M_W_reg.null = 1;	
		return;
	}
	cout << "		MemOp: " << E_M_reg.controls.MemOp << endl;

	instruction_register = E_M_reg.instruction;
	M_W_reg.instruction = E_M_reg.instruction;
	M_W_reg.pc = E_M_reg.pc;
	M_W_reg.ALUresult = E_M_reg.ALUresult;
	M_W_reg.null=0;

	cout << "		instruction_register : ";
	
	printf("0x%08X\n		PC:0x%X\n%c", instruction_register,M_W_reg.pc, flush_inp_out(););
	fprintf(out,"MA:%d\n",(M_W_reg.pc/4)+1);
	if(M_W_reg.pc == EXIT_pc){
		M_W_reg.null = 0;
		return;
	}
	if(E_M_reg.controls.MemOp != NoMEMOp){
		
		Data_transfers++;
		switch(E_M_reg.controls.MemOp){
			//loads byte
			case(MEM_lb):{
				datacache(0, E_M_reg.ALUresult, 0);
				M_W_reg.ld_result = output_byte;
				printf("		loaded :  %02x",M_W_reg.ld_result);
				break;
			}
			//loads half word
			case(MEM_lh):{
				datacache(0, E_M_reg.ALUresult, 1);
				M_W_reg.ld_result = output_halfword;
				printf("		loaded :  %02x",M_W_reg.ld_result);
				break;
			}
			//loads word
			case(MEM_lw):{
				datacache(0, E_M_reg.ALUresult, 2);
				M_W_reg.ld_result = output_word;
				printf("		loaded :  %02x",M_W_reg.ld_result);
				break;
			}
			//stores word	
			case(MEM_sw):{
				input_word = E_M_reg.op2;
				printf("input_word = %0x\nE_M_result = %0x\n", input_word, E_M_reg.ALUresult);
				datacache(1, E_M_reg.ALUresult, 2);
				cout << "		stored :   " ;
				printf( "				%08X\n  		%02x %02x %02x %02x  ",E_M_reg.ALUresult,DMEM[E_M_reg.ALUresult+3],DMEM[E_M_reg.ALUresult+2],DMEM[E_M_reg.ALUresult+1],DMEM[E_M_reg.ALUresult]);
				break;
			}
			//stores half word
			case(MEM_sh):{
				input_halfword = E_M_reg.op2;
				datacache(1, E_M_reg.ALUresult, 1);
				cout << "		Stored : " ;
				printf( " %02x %02x ",DMEM[E_M_reg.ALUresult],DMEM[E_M_reg.ALUresult+1]);
				break;
			}
			//stores byte
			case(MEM_sb):{
				input_byte = E_M_reg.op2;
				datacache(1, E_M_reg.ALUresult, 0);
				cout << "		Stored : " ;
				printf( "  %02x  ",DMEM[E_M_reg.ALUresult]);
				break;
			}
		}
	}
	M_W_reg.controls = E_M_reg.controls;
}

//writes the results back to register file
void write_back() {
	
	instruction_register = M_W_reg.instruction;
	if(M_W_reg.null)
	{
		if(extract_bits(0,6)==EXIT){
				swi_exit();
		}
		cout << "\n\nWrite_back:\n";
		cout << "             []\n";
		fprintf(out1,"%d\n",cycle_no+1);
		fprintf(out,"\nWB:\n");
		return;
	}
		fprintf(out1,"%d\n",cycle_no+1);
	in_ex++;
	int dum = extract_bits(0,6);
	if(dum == Jtype || dum == UtypeA || dum == UtypeL)
		ALU_inst--;
	
	cout << "\n\nWrite_back:\n";
	
	cout << "		instruction_register : ";
	printf("0x%08X\n		PC:0x%X\n", instruction_register,M_W_reg.pc);
	fprintf(out,"\nWB:%d\n",(M_W_reg.pc/4)+1);
	cout << "		RFWrite :" << M_W_reg.controls.RFWrite<<endl;
	regdestiny = extract_bits(7,11);
	if(M_W_reg.controls.RFWrite){
		if(regdestiny){
			switch(M_W_reg.controls.ResultSelect){
				case(From_ALU):{
					X[regdestiny] = M_W_reg.ALUresult;
					break;
				}
				case(From_ImmU):{
					X[regdestiny] = M_W_reg.ALUresult; 
					break;
				}
				case(From_MEM):{
					X[regdestiny] = M_W_reg.ld_result; 
					break;
				}
				case(From_PC):{
					X[regdestiny] = M_W_reg.pc + 4; 
					break;
				}
				case(From_AUIPC):{
					X[regdestiny] = M_W_reg.pc + M_W_reg.ALUresult;
					break;
				}
			}

			cout <<"		rd: X" << regdestiny<<endl;
			cout << "		X[rd]:" << X[regdestiny] << endl;
			
			fprintf(out1,"X%d:0x%08X\n",regdestiny,X[regdestiny]);
		}
		else
		cout << "		rd: X0"<<endl;
	}

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

void reg_file(){
	fprintf(rfile,"\nCycle No. = %d\n",cycle_no);
	for(int i = 0;i<32;i++)
	{
		fprintf(rfile,"X[%d] = 0x%08X\n",i,X[i]);
	}
}

void pipe_reg(){
	fprintf(pfile,"------------------------------------\nCycle No. = %d\n",cycle_no);

	fprintf(pfile,"\nFETCH-DECODE pipeline register\n");
	if(F_D_reg.null){
		fprintf(pfile,"NULL\n");
	}
	else{
	fprintf(pfile,"PC = %d  0x%08X\n",F_D_reg.pc,F_D_reg.pc);
	fprintf(pfile,"INSTRUCTION = 0x%08X\n",F_D_reg.instruction);
	}
	fprintf(pfile,"\nDECODE-EXECUTION pipeline register\n");
	if(D_E_reg.null){
		fprintf(pfile,"NULL\n");
	}
	else{
		fprintf(pfile,"PC = %d  0x%08X\n",D_E_reg.pc,D_E_reg.pc);
		fprintf(pfile,"INSTRUCTION = 0x%08X\n",D_E_reg.instruction);
		fprintf(pfile,"OPERAND1 = %d\n",D_E_reg.operand1);
		fprintf(pfile,"OPERAND2 = %d\n",D_E_reg.operand2);
		fprintf(pfile,"OP2 = %d\n",D_E_reg.op2);
		fprintf(pfile,"BRANCHTARGET = %d\n",D_E_reg.BranchTarget_Addr);
		fprintf(pfile,"Control Signals:\n");
		fprintf(pfile,"ALUsignal = %d\n",D_E_reg.controls.ALUOp);
		fprintf(pfile,"IsBranch = %d\n",D_E_reg.controls.IsBranch);
		fprintf(pfile,"Branchtype = %d\n",D_E_reg.controls.BranchType);
	}
	fprintf(pfile,"\nEXECUTION-MEMORY pipeline register\n");

	if(E_M_reg.null){
		fprintf(pfile,"NULL\n");
	}
	else{
		fprintf(pfile,"PC = %d  0x%08X\n",E_M_reg.pc,E_M_reg.pc);
		fprintf(pfile,"INSTRUCTION = 0x%08X\n",E_M_reg.instruction);
		fprintf(pfile,"ALUResult = %d\n",E_M_reg.ALUresult);
		fprintf(pfile,"OP2 = %d\n",E_M_reg.op2);
		fprintf(pfile,"Control Signals:\n");
		fprintf(pfile,"Memory signal = %d\n",E_M_reg.controls.MemOp);
	}

		fprintf(pfile,"\nMEMORY-WRITEBACK pipeline register\n");
	if(M_W_reg.null){
		fprintf(pfile,"NULL\n");
	}
	else{
		fprintf(pfile,"PC = %d  0x%08X\n",M_W_reg.pc,M_W_reg.pc);
		fprintf(pfile,"INSTRUCTION = 0x%08X\n",M_W_reg.instruction);
		fprintf(pfile,"LOADResult = %d\n",M_W_reg.ld_result);
		fprintf(pfile,"ALUResult = %d\n",M_W_reg.ALUresult);
		fprintf(pfile,"Control Signals:\n");
		fprintf(pfile,"IsWriteBack = %d\n",M_W_reg.controls.RFWrite);
		fprintf(pfile,"Result select = %d\n",M_W_reg.controls.ResultSelect);		
	}

}

void viewDMEM(){

    vector<pair< int , unsigned char> > vec;

    for(auto& it : DMEM){
        vec.push_back(it);
    }
    sort(vec.begin(),vec.end(),comp);
    if(DMEM.size())
    fprintf(out1,"--- MEMORY ---\n");
    else
    fprintf(out1,"-\n");
    for(auto& m : vec){
        if(!(m.first%4))
        fprintf(out1, "0x%08X:%02X %02X %02X %02X\n",m.first,DMEM[m.first],DMEM[m.first+1],DMEM[m.first+2],DMEM[m.first+3]);
    }
    fprintf(out1,"\n");
}

void print_inst(unsigned int _pc,unsigned int _ins,FILE *t)
{	int opcode, func3, rd, rs1, rs2;
	instruction_register = _ins;
	
	fprintf(t,"%d:",(_pc/4) + 1);
	opcode = extract_bits(0,6);

	rs1 = extract_bits(15,19);	
	rd = extract_bits(7,11);
	rs2 = extract_bits(20,24);
	func3 = extract_bits(12,14);

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


	int immU = extract_bits(12,31);
	immU = sign_extender(immU, 19) << 12;
	

	int immJ = extract_bits(31,31) << 20;
	immJ += (extract_bits(12,19) << 12);
	immJ += (extract_bits(20,20) << 11);	
	immJ += (extract_bits(21,30) << 1);
	immJ = sign_extender(immJ, 20);

	switch(opcode){
		// I-type - Arithmetic
		case(ItypeA):
		// R-type
		case(Rtype):{
	
			switch(func3){
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
			fprintf(t," x%d %d(x%d)",rs2,immS,rs1);
			break;
		}
	
		// B-type
		case(Btype):{
			switch(func3){
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
			fprintf(t," x%d x%d %d",rs1,rs2,immB);
			break;
		}
		
		// J-type
		case(Jtype):{
			fprintf(t,"JAL x%d %d",rd,immJ);
			break;
		}
	
		// U-type - auipc
		case(UtypeA):{
			fprintf(t,"AUIPC x%d 0x%x",rd ,immU>>12);
			break;
		}

		// U-type - lui
		case(UtypeL):{
			fprintf(t,"LUI x%d 0x%x",rd,immU>>12);
			break;
		}
		case(EXIT):{
			fprintf(t,"EXIT");
			return;
		}
		
	}
	fprintf(t,"\n");
}

void write_to_DMEM()
{
	for(int i=0; i<data_cachelines; i++)
	{
		if(data_cache[i].update == 1)
		{
			int index;
			if(data_mapping == 0) // Direct 
			{
				index = i;
			}
			else if(data_mapping == 1) // Full
			{
				index = 1;
			}
			else // Set
			{
				index = i % data_ways;
			}
			int offset = log2(data_blocksize);
			for(int j=0; j<data_blocksize; j++)
			{
				unsigned int location = (data_cache[i].tag << (unsigned)(log2(index)+offset));
				if(data_mapping != 1)
				{
					location += (index<<offset);
				}
				location += j;
				DMEM[location] = data_cache[i].words[j];
			}
		}
	}
}

void free_malloc()
{
	for(int i=0; i<data_cachelines; i++)
	{
		free(data_cache[i].words);
	}
	for(int i=0; i<ins_cachelines; i++)
	{
		free(ins_cache[i].words);
	}
}

void datacache(int opr, int addr, int type)
{
	data_accesses++;
	if(data_mapping == 0) // Direct
	{
		unsigned int index = addr;
		unsigned int tag = addr;
		unsigned int blockoff = addr;
		int x = log2(data_cachelines);
		int y = log2(data_blocksize);
		index = index<<(32-x-y);
		index = index>>(32-x);
		tag = tag>>(x+y);
		blockoff = blockoff<<(32-y);
		blockoff = blockoff>>(32-y);

		if(data_cache[index].valid == 1)
		{
			if(data_cache[index].tag == tag)
			{
				data_hits++;
				if(opr == 0)
				{
					if(type == 0)
					{
						output_byte = data_cache[index].words[blockoff];
						return;
					}
					else if(type == 1)
					{
						output_halfword = data_cache[index].words[blockoff+1];
						output_halfword = output_halfword<<8;
						output_halfword += data_cache[index].words[blockoff];
						return;
					}
					else if(type == 2)
					{
						output_word = data_cache[index].words[blockoff+3];
						output_word = output_word<<8;
						output_word += data_cache[index].words[blockoff+2];
						output_word = output_word<<8;
						output_word += data_cache[index].words[blockoff+1];
						output_word = output_word<<8;
						output_word += data_cache[index].words[blockoff];
						return;
					}
				}
				else if(opr == 1)
				{
					data_cache[index].update = 1;
					data_cache[index].tag = tag;
					if(type == 0)
					{
						data_cache[index].words[blockoff] = input_byte;
						return;
					}
					else if(type == 1)
					{
						data_cache[index].words[blockoff] = input_halfword;
						data_cache[index].words[blockoff+1] = input_halfword>>8;
						return;
					}
					else if(type == 2)
					{
						data_cache[index].words[blockoff+3] = input_word>>24;
						data_cache[index].words[blockoff+2] = ((input_word<<8)>>24);
						data_cache[index].words[blockoff+1] = ((input_word<<16)>>24);
						data_cache[index].words[blockoff] = input_word;
						return;
					}
				}
			}
			else
			{
				if(opr == 0)
				{
					data_conflict++;
					if(data_cache[index].update == 1)
					{
						unsigned int temp = (data_cache[index].tag)<<(x+y);
						temp += (index<<y);
						for(int i=0; i<data_blocksize; i++)
						{
							DMEM[temp+i] = data_cache[index].words[i];
						}
						data_cache[index].update = 0;
					}

					unsigned int temp = (addr>>y)<<y;
					for(int i=0; i<data_blocksize; i++)
					{
						data_cache[index].words[i] = DMEM[temp+i];
					}
					data_cache[index].valid = 1;
					data_cache[index].tag = tag;

					if(type == 0)
					{
						output_byte = data_cache[index].words[blockoff];
						return;
					}
					else if(type == 1)
					{
						output_halfword = data_cache[index].words[blockoff+1];
						output_halfword = output_halfword<<8;
						output_halfword += data_cache[index].words[blockoff];
						return;
					}
					else if(type == 2)
					{
						output_word = data_cache[index].words[blockoff+3];
						output_word = output_word<<8;
						output_word += data_cache[index].words[blockoff+2];
						output_word = output_word<<8;
						output_word += data_cache[index].words[blockoff+1];
						output_word = output_word<<8;
						output_word += data_cache[index].words[blockoff];
						return;
					}
				}
				else if(opr == 1)
				{
					data_writemisses++;
					if(type == 0)
					{
						DMEM[addr] = input_byte;
						return;
					}
					else if(type == 1)
					{
						DMEM[addr] = input_halfword;
						DMEM[addr+1] = input_halfword>>8;
						return;
					}
					else if(type == 2)
					{
						DMEM[addr] = input_word;
						DMEM[addr+1] = ((input_word<<16)>>24);					
						DMEM[addr+2] = ((input_word<<8)>>24);
						DMEM[addr+3] = input_word>>24;
						return;
					}
				}
			}
		}
		else
		{
			if(opr == 0)
			{
				data_cold++;
				unsigned int temp = (addr>>y)<<y;
				for(int i=0; i<data_blocksize; i++)
				{
					data_cache[index].words[i] = DMEM[temp+i];
				}
				data_cache[index].valid = 1;
				data_cache[index].tag = tag;

				if(type == 0)
				{
					output_byte = data_cache[index].words[blockoff];
					return;
				}
				else if(type == 1)
				{
					output_halfword = data_cache[index].words[blockoff+1];
					output_halfword = output_halfword<<8;
					output_halfword += data_cache[index].words[blockoff];
					return;
				}
				else if(type == 2)
				{
					output_word = data_cache[index].words[blockoff+3];
					output_word = output_word<<8;
					output_word += data_cache[index].words[blockoff+2];
					output_word = output_word<<8;
					output_word += data_cache[index].words[blockoff+1];
					output_word = output_word<<8;
					output_word += data_cache[index].words[blockoff];
					return;
				}
			}
			else if(opr == 1)
			{
				data_writemisses++;
				if(type == 0)
				{
					DMEM[addr] = input_byte;
					return;
				}
				else if(type == 1)
				{
					DMEM[addr] = input_halfword;
					DMEM[addr+1] = input_halfword>>8;
					return;
				}
				else if(type == 2)
				{
					DMEM[addr] = input_word;
					DMEM[addr+1] = ((input_word<<16)>>24);
					DMEM[addr+2] = ((input_word<<8)>>24);
					DMEM[addr+3] = (input_word>>24);
					return;
				}
			}
		}
	}
	else if(data_mapping == 1) // Full
	{
		unsigned int tag = addr;
		unsigned int blockoff = addr;
		int y = log2(data_blocksize);
		tag = tag>>(y);
		blockoff = blockoff<<(32-y);
		blockoff = blockoff>>(32-y);
		int miss=1;
		int lineno=-1;
		int capacity = 1;

		for(int i=0; i<data_cachelines; i++)
		{
			if(data_cache[i].tag == tag)
			{
				if(data_cache[i].valid == 1)
				{
					miss = 0;
					lineno = i;
				}
				else
				{
					capacity = 0;
				}
			}
		}

		if(miss == 0)
		{
			data_hits++;
			if(data_replacepolicy == 0)
			{
				for(int i=0; i<data_cachelines; i++)
				{
					if(data_cache[i].RF > data_cache[lineno].RF)
					{
						data_cache[i].RF--;
					}
				}
				data_cache[lineno].RF = data_cachelines-1;
			}

			if(opr == 0)
			{
				if(type == 0)
				{
					output_byte = data_cache[lineno].words[blockoff];
					return;
				}
				else if(type == 1)
				{
					output_halfword = data_cache[lineno].words[blockoff+1];
					output_halfword = output_halfword<<8;
					output_halfword += data_cache[lineno].words[blockoff];
					return;
				}
				else if(type == 2)
				{
					output_word = data_cache[lineno].words[blockoff+3];
					output_word = output_word<<8;
					output_word += data_cache[lineno].words[blockoff+2];
					output_word = output_word<<8;
					output_word += data_cache[lineno].words[blockoff+1];
					output_word = output_word<<8;
					output_word += data_cache[lineno].words[blockoff];
					return;
				}
			}
			else if(opr == 1)
			{
				data_cache[lineno].update = 1;
				if(type == 0)
				{
					data_cache[lineno].words[blockoff] = input_byte;
					return;
				}
				else if(type == 1)
				{
					data_cache[lineno].words[blockoff] = input_halfword;
					data_cache[lineno].words[blockoff+1] = input_halfword>>8;
					return;
				}
				else if(type == 2)
				{
					data_cache[lineno].words[blockoff] = input_word;
					data_cache[lineno].words[blockoff+1] = ((input_word<<16)>>16);
					data_cache[lineno].words[blockoff+2] = ((input_word<<8)>>24);
					data_cache[lineno].words[blockoff+3] = input_word>>24;
					return;
				}
			}
		}
		else if(miss == 1)
		{
			if(opr == 0)
			{
				if(capacity == 0)
				{
					data_cold++;
				}
				else
				{
					data_capacity++;
				}
				int replaceline=-1;
				if(data_replacepolicy == 0)
				{
					for(int i=0; i<data_cachelines; i++)
					{
						if(data_cache[i].RF == 0)
						{
							replaceline = i;
						}
					}
					for(int i=0; i<data_cachelines; i++)
					{
						data_cache[i].RF--;
					}
					data_cache[replaceline].RF = data_cachelines-1;
				}
				else if(data_replacepolicy == 1)
				{
					for(int i=0; i<data_cachelines; i++)
					{
						if(data_cache[i].RF == 1)
						{
							replaceline = i;
							data_cache[i].RF = 0;
						}
					}
					if(replaceline == data_cachelines-1)
					{
						data_cache[0].RF = 1;
					}
					else
					{
						data_cache[replaceline+1].RF = 1;
					}
				}
				else if(data_replacepolicy == 2)
				{
					int x=0;
					int r = rand();
					r = r%data_cachelines;
					int p = 0;
					replaceline = -1;

					for(int i=0; i<data_cachelines; i++)
					{
						if(data_cache[i].valid == 0)
							p++;
					}
					if(p == 1)
					{
						p=0;
						while(r >= 0)
						{
							if(data_cache[p].valid == 0)
							{
								replaceline=p;
								r--;
							}
							p++;
							if(p == data_cachelines)
							{
								p=0;
							}
						}
					}
					else
					{
						replaceline = r;
					} 
				}

				if(data_cache[replaceline].update == 1)
				{
					unsigned int temp = (data_cache[replaceline].tag)<<(y);
					temp = (temp>>y)<<y;
					for(int i=0; i<data_blocksize; i++)
					{
						DMEM[temp+i] = data_cache[replaceline].words[i];
					}
					data_cache[replaceline].update = 0;
				}

				unsigned int temp = (addr>>y)<<y;
				for(int i=0; i<data_blocksize; i++)
				{
					data_cache[replaceline].words[i] = DMEM[temp+i];
				}
				data_cache[replaceline].valid = 1;
				data_cache[replaceline].tag = tag;

				if(type == 0)
				{
					output_byte = data_cache[replaceline].words[blockoff];
					return;
				}
				else if(type == 1)
				{
					output_halfword = data_cache[replaceline].words[blockoff+1];
					output_halfword = output_halfword<<8;
					output_halfword += data_cache[replaceline].words[blockoff];
					return;
				}
				else if(type == 2)
				{
					output_word = data_cache[replaceline].words[blockoff+3];
					output_word = output_word<<8;
					output_word += data_cache[replaceline].words[blockoff+2];
					output_word = output_word<<8;
					output_word += data_cache[replaceline].words[blockoff+1];
					output_word = output_word<<8;
					output_word += data_cache[replaceline].words[blockoff];
					return;
				}
			}
			else if(opr == 1)
			{
				data_writemisses++;
				if(type == 0)
				{
					DMEM[addr] = input_byte;
					return;
				}
				else if(type == 1)
				{
					DMEM[addr] = input_halfword;
					DMEM[addr+1] = input_halfword>>8;
					return;
				}
				else if(type == 2)
				{
					DMEM[addr] = input_word;
					DMEM[addr+1] = ((input_word<<16)>>24);
					DMEM[addr+2] = ((input_word<<8)>>24);
					DMEM[addr+3] = input_word>>24;
					return;
				}
			}
		}
	}
	else if(data_mapping == 2) // Set way
	{
		unsigned int index = addr;
		unsigned int tag = addr;
		unsigned int blockoff = addr;
		int x = log2(data_cachelines);
		int y = log2(data_blocksize);
		int z = log2(data_ways);
		index = index<<(32-x-y+z);
		index = index>>(32-x+z);
		tag = tag>>(x+y-z);
		blockoff = blockoff<<(32-y);
		blockoff = blockoff>>(32-y);
		int miss=1;
		int lineno=-1;
		int capacity = 1;

		index = index * data_ways;

		for(int i=0; i<data_ways; i++)
		{
			if(data_cache[index+i].tag == tag)
			{
				data_hits++;
				if(data_cache[index+i].valid == 1)
				{
					miss = 0;
					lineno = index+i;
				}
				else
				{
					capacity = 0;
				}
			}
		}

		if(miss == 0)
		{
			data_hits++;
			if(data_replacepolicy == 0)
			{
				for(int i=index; i<index + data_ways; i++)
				{
					if(data_cache[i].RF > data_cache[lineno].RF)
					{
						data_cache[i].RF--;
					}
				}
				data_cache[lineno].RF = data_cachelines-1;
			}

			if(opr == 0)
			{
				if(type == 0)
				{
					output_byte = data_cache[lineno].words[blockoff];
					return;
				}
				else if(type == 1)
				{
					output_halfword = data_cache[lineno].words[blockoff+1];
					output_halfword = output_halfword<<8;
					output_halfword += data_cache[lineno].words[blockoff];
					return;
				}
				else if(type == 2)
				{
					output_word = data_cache[lineno].words[blockoff+3];
					output_word = output_word<<8;
					output_word += data_cache[lineno].words[blockoff+2];
					output_word = output_word<<8;
					output_word += data_cache[lineno].words[blockoff+1];
					output_word = output_word<<8;
					output_word += data_cache[lineno].words[blockoff];
					return;
				}
			}
			else if(opr == 1)
			{
				
				data_cache[lineno].update = 1;
				if(type == 0)
				{
					data_cache[lineno].words[blockoff] = input_byte;
					return;
				}
				else if(type == 1)
				{
					data_cache[lineno].words[blockoff] = input_halfword;
					data_cache[lineno].words[blockoff+1] = input_halfword>>8;
					return;
				}
				else if(type == 2)
				{
					data_cache[lineno].words[blockoff] = input_word;
					data_cache[lineno].words[blockoff+1] = ((input_word<<16)>>24);
					data_cache[lineno].words[blockoff+2] = ((input_word<<8)>>24);
					data_cache[lineno].words[blockoff+3] = input_word>>24;
					return;
				}
			}
		}
		else
		{
			if(opr == 0)
			{
				if(capacity == 0)
				{
					data_cold++;
				}
				else
				{
					data_conflict++;
				}

				int replaceline=-1;
				if(data_replacepolicy == 0)
				{
					for(int i=0; i<data_ways; i++)
					{
						if(data_cache[index + i].RF == 0)
						{
							replaceline = index + i;
						}
					}
					for(int i=0; i<data_ways; i++)
					{
						data_cache[index + i].RF--;
					}
					data_cache[replaceline].RF = data_ways-1;
				}
				else if(data_replacepolicy == 1)
				{
					for(int i=0; i<data_ways; i++)
					{
						if(data_cache[index + i].RF == 1)
						{
							replaceline = index + i;
							data_cache[index + i].RF = 0;
						}
					}
					if(replaceline == index + data_ways - 1)
					{
						data_cache[index].RF = 1;
					}
					else
					{
						data_cache[replaceline + 1].RF = 1;
					}
				}
				else if(data_replacepolicy == 2)
				{
					int x=0;
					int r = rand();
					r = r%data_ways;
					int p = 0;
					replaceline = -1;

					for(int i=0; i<data_ways; i++)
					{
						if(data_cache[index + i].valid == 0)
							p++;
					}
					if(p == 1)
					{
						p=0;
						while(r >= 0)
						{
							if(data_cache[index + p].valid == 0)
							{
								replaceline = index + p;
								r--;
							}
							p++;
							if(p == data_ways)
							{
								p=0;
							}
						}
					}
					else
					{
						replaceline = r;
					}
				}

				if(data_cache[replaceline].update == 1)
				{
					unsigned int temp = (data_cache[replaceline].tag)<<(x+y);
					temp += replaceline<<y;
					for(int i=0; i<data_blocksize; i++)
					{
						DMEM[temp+i] = data_cache[replaceline].words[i];
					}
					data_cache[replaceline].update = 0;
				}

				unsigned int temp = (addr>>y)<<y;
				for(int i=0; i<data_blocksize; i++)
				{
					data_cache[replaceline].words[i] = DMEM[temp+i];
				}
				data_cache[replaceline].tag = tag;
				data_cache[replaceline].valid = 1;

				if(type == 0)
				{
					output_byte = data_cache[replaceline].words[blockoff];
					return;
				}
				else if(type == 1)
				{
					output_halfword = data_cache[replaceline].words[blockoff+1];
					output_halfword = output_halfword<<8;
					output_halfword += data_cache[replaceline].words[blockoff];
					return;
				}
				else if(type == 2)
				{
					output_word = data_cache[replaceline].words[blockoff+3];
					output_word = output_word<<8;
					output_word += data_cache[replaceline].words[blockoff+2];
					output_word = output_word<<8;
					output_word += data_cache[replaceline].words[blockoff+1];
					output_word = output_word<<8;
					output_word += data_cache[replaceline].words[blockoff];
					return;
				}
			}
			else if(opr == 1)
			{
				data_writemisses++;
				if(type == 0)
				{
					DMEM[addr] = input_byte;
					return;
				}
				else if(type == 1)
				{
					DMEM[addr] = input_halfword;
					DMEM[addr+1] = input_halfword>>8;
					return;
				}
				else if(type == 2)
				{
					DMEM[addr] = input_word;
					input_word = input_word - ((input_word>>24)<<24);
					DMEM[addr+1] = ((input_word<<16)>>24);
					input_word = input_word - ((input_word>>16)<<16);
					DMEM[addr+2] = ((input_word<<8)>>24);
					input_word = input_word - ((input_word>>8)<<8);
					DMEM[addr+3] = input_word>>24;
					return;
				}
			}
		}
	}
}

int inscache(unsigned int addr)
{
	if(E_M_reg.bp_result == true)
	{
		if(HAZARD)
		 return 0;
	}
	ins_accesses++;
	int return_value=0;
	if(ins_mapping == 0) // Direct
	{
		unsigned int index = addr;
		unsigned int tag = addr;
		unsigned int blockoff = addr;
		int x = log2(ins_cachelines);
		int y = log2(ins_blocksize);
		index = index<<(30-x-y);
		index = index>>(32-x);
		tag = tag>>(x+y+2);
		blockoff = blockoff<<(30-y);
		blockoff = blockoff>>(32-y);

		if(ins_cache[index].valid == 1)
		{
			if(ins_cache[index].tag == tag)
			{
				ins_hits++;
				return ins_cache[index].words[blockoff];
			}
			else
			{
				ins_conflict++;
				ins_misses++;
				int temp = (addr>>(y+2));
				printf("--%d--",temp);
				temp = temp<<(y+2);
					printf("--%d--",temp);
				int* tmp = (int *)&MEM[temp];
				for(int i=0; i<ins_blocksize; i++)
				{
					ins_cache[index].words[i] = *(tmp+i);
				}
				ins_cache[index].tag = tag;
				printf("ins_cache: %d %x  \n\n", index, temp);
				for(int i=0; i<ins_cachelines; i++)
				{
					for(int j=0; j<ins_blocksize; j++)
					{
						printf("0x%08x  ", ins_cache[i].words[j]);
					}
					printf("\n");
				}
				return ins_cache[index].words[blockoff];
			}
		}
		else
		{
			ins_cold++;
			ins_misses++;
			int temp = (addr>>(y+2))<<(y+2);
			int* tmp = (int *)&MEM[temp];
			for(int i=0; i<ins_blocksize; i++)
			{
				ins_cache[index].words[i] = *(tmp+i);
			}
			ins_cache[index].valid = 1;
			ins_cache[index].tag = tag;	
			for(int i=0; i<ins_cachelines; i++)
			{
				for(int j=0; j<ins_blocksize; j++)
				{
					printf("0x%08x  ", ins_cache[i].words[j]);
				}
				printf("\n");
			}		
			return ins_cache[index].words[blockoff];
		}
	}
	else if(ins_mapping == 1) // Full
	{
		unsigned int tag = addr;
		unsigned int blockoff = addr;
		int y = log2(ins_blocksize);
		tag = tag>>(y+2);
		blockoff = blockoff<<(30-y);
		blockoff = blockoff>>(32-y);
		int miss=1;
		int lineno=-1;
		int capacity=1;

		for(int i=0; i<ins_cachelines; i++)
		{
			if(ins_cache[i].valid == 1)
			{
				if(ins_cache[i].tag == tag)
				{
					miss = 0;
					lineno = i;	
				}
			}
			else
			{
				capacity=0;
			}
		}

		if(miss == 0)
		{
			ins_hits++;
			if(ins_replacepolicy == LRU)
			{
				for(int i=0; i<ins_cachelines; i++)
				{
					if(ins_cache[i].RF > ins_cache[lineno].RF)
					{
							ins_cache[i].RF--;
					}
				}
				ins_cache[lineno].RF = ins_cachelines-1;
			}
			return ins_cache[lineno].words[blockoff];
		}

		else if(miss == 1)
		{
			if(capacity == 0)
			{
				ins_cold++;
				ins_misses++;
			}
			else
			{
				ins_capacity++;
				ins_misses++;
			}

			int replaceline = -1;
			if(ins_replacepolicy == LRU)
			{
				for(int i=0; i<ins_cachelines; i++)
				{
					if(ins_cache[i].RF == 0)
					{
						replaceline = i;
						break;
					}
				}
				for(int i=0; i<ins_cachelines; i++)
				{
					ins_cache[i].RF--;
				}
				ins_cache[replaceline].RF = ins_cachelines-1;
			}
			else if(ins_replacepolicy == FIFO)
			{
				for(int i=0; i<ins_cachelines; i++)
				{
					if(ins_cache[i].RF == 1)
					{
						replaceline = i;
						ins_cache[i].RF = 0;
						break;
					}
				}
				if(replaceline == ins_cachelines-1)
				{
					ins_cache[0].RF = 1;
				}
				else
				{
					ins_cache[replaceline+1].RF = 1;
				}
			}
			else if(ins_replacepolicy == 2)
			{
				int x=0;
				int r = rand();
				r = r%ins_cachelines;
				int p = 0;

				for(int i=0; i<ins_cachelines; i++)
				{
					if(ins_cache[i].valid == 0)
					{
						p++;
						break;
					}
				}
				if(p == 1)
				{
					p=0;
					while(r >= 0)
					{
						if(ins_cache[p].valid == 0)
						{
							replaceline=p;
							r--;
						}
						p++;
						if(p == ins_cachelines-1)
						{
							p=0;
						}
					}
				}
				else
				{
					replaceline = r;
				} 
			}

			int temp = (addr>>(y+2))<<(y+2);
			int* tmp = (int *)&MEM[temp];
			for(int i=0; i<ins_blocksize; i++)
			{
				ins_cache[replaceline].words[i] = *(tmp+i);
			}
			ins_cache[replaceline].valid = 1;
			ins_cache[replaceline].tag = tag;
			return ins_cache[replaceline].words[blockoff];
		}
	}
	else if(ins_mapping == 2) // Set
	{
		unsigned int index = addr;
		unsigned int tag = addr;
		unsigned int blockoff = addr;
		int x = log2(ins_cachelines);
		int y = log2(ins_blocksize);
		int z = log2(ins_ways);
		index = index<<(30-x-y+z);
		index = index>>(32-x+z);
		tag = tag>>(x+y-z+2);
		blockoff = blockoff<<(30-y);
		blockoff = blockoff>>(32-y);
		int miss=1;
		int lineno=-1;
		int capacity = 1;

		index = index * ins_ways;

		for(int i=0; i<ins_ways; i++)
		{
			if(ins_cache[index+i].valid == 1)
			{
				if(ins_cache[index+i].tag == tag)
				{
					miss = 0;
					lineno = index+i;
				}	
			}
			else
			{
				capacity = 0;
			}
		}

		if(miss == 0)
		{
			ins_hits++;
			if(ins_replacepolicy == LRU)
			{
				for(int i=index; i<index + ins_ways; i++)
				{
					if(ins_cache[i].RF > ins_cache[lineno].RF)
					{
						ins_cache[i].RF--;
					}
					
				}
				ins_cache[lineno].RF = ins_cachelines-1;
			}
			return ins_cache[lineno].words[blockoff];
		}
		else
		{
			if(capacity == 0)
			{
				ins_cold++;
				ins_misses++;
			}
			else
			{
				ins_conflict++;
				ins_misses++;
			}
			int replaceline=-1;
			if(ins_replacepolicy == LRU)
			{
				for(int i=0; i<ins_ways; i++)
				{
					if(ins_cache[index + i].RF == 0)
					{
						replaceline = index + i;
						break;
					}
				}
				for(int i=0; i<ins_ways; i++)
				{
					ins_cache[index + i].RF--;
				}
				ins_cache[replaceline].RF = ins_ways-1;
			}
			else if(ins_replacepolicy == FIFO)
			{
				for(int i=0; i<ins_ways; i++)
				{
					if(ins_cache[index + i].RF == 1)
					{
						replaceline = index + i;
						ins_cache[index + i].RF = 0;
						break;
					}
				}
				if(replaceline == index + ins_ways - 1)
				{
					ins_cache[index].RF = 1;
				}
				else
				{
					ins_cache[replaceline + 1].RF = 1;
				}
			}
			else if(ins_replacepolicy == 2)
			{
				int x=0;
				int r = rand();
				r = r%ins_ways;
				int p = 0;

				for(int i=0; i<ins_ways; i++)
				{
					if(ins_cache[index + i].valid == 0)
					{
						p++;
						break;
					}
				}
				if(p == 1)
				{
					p=0;
					while(r >= 0)
					{
						if(ins_cache[index + p].valid == 0)
						{
							replaceline = index + p;
							r--;
						}
						p++;
						if(p == ins_ways)
						{
							p=0;
						}
					}
				}
				else
				{
					replaceline = r;
				}
			}

			int temp = (addr>>(y+2))<<(y+2);
			int* tmp = (int *)&MEM[temp];
			for(int i=0; i<ins_blocksize; i++)
			{
				ins_cache[replaceline].words[i] = *(tmp+i);
			}
			ins_cache[replaceline].valid = 1;
			ins_cache[replaceline].tag = tag;
			return ins_cache[replaceline].words[blockoff];
		}
	}
}