import os
import webbrowser
from flask import Flask, render_template, request, redirect

app = Flask(__name__)

RF = {"X0":"0x00000000",
"X1":"0x00000000","X2":"0x7FFFFFDC",
"X3":"0x00000000","X4":"0x00000000",
"X5":"0x00000000","X6":"0x00000000",
"X7":"0x00000000","X8":"0x00000000",
"X9":"0x00000000","X10":"0x00000000",
"X11":"0x00000000","X12":"0x00000000",
"X13":"0x00000000","X14":"0x00000000",
"X15":"0x00000000","X16":"0x00000000",
"X17":"0x00000000","X18":"0x00000000",
"X19":"0x00000000","X20":"0x00000000",
"X21":"0x00000000","X22":"0x00000000",
"X23":"0x00000000","X24":"0x00000000",
"X25":"0x00000000","X26":"0x00000000",
"X27":"0x00000000","X28":"0x00000000",
"X29":"0x00000000","X30":"0x00000000",
"X31":"0x00000000",}

RFinit = RF.copy()
instructions = {}
out = open("./OUTPUT.txt", "r")
out2 = open("./MEM.txt", "r")
memory={}
cycles={"0":0, "max":0}
inst_no=0
pipeline_data = {}
instruction_index={}
branch_details={}
run_status=0;
chazards = {}
dhazards = {}

def SingleSim():
	os.system("cd ../src && make")
	os.system("../bin/myRISCVSim ../test/input.mc")

def PipeSim():
	os.system("cd ../src_pipe && make")
	os.system("../bin/myRISCVpipe ../test/input.mc")

@app.route("/")
def home():
	instructions.clear()
	memory.clear()
	pipeline_data.clear()
	branch_details.clear()
	instruction_index.clear()
	RF = RFinit
	cycles["0"] = 0
	cycles["max"] = 0
	run_status=0
	out.seek(0)
	out2.seek(0)
	return render_template('home.html')


@app.route("/run", methods=["POST"])
def run():
	if request.method == "POST":
		with open("./knobs.txt", "w") as f:
			if request.form.get("dataf"):
				f.write("1\n")
			else:
				f.write("0\n")
			if request.form.get("printrf"):
				f.write("1\n")
			else:
				f.write("0\n")
			if request.form.get("printpr"):
				f.write("1\n")
			else:
				f.write("0\n")
			if request.form.get("5th"):
				f.write("1\n")
			else:
				f.write("0\n")
		if not request.form.get("pipeline"):
			return redirect("/single")
		else:
			return redirect("/pipeline")
	else:
		return redirect("/")

@app.route("/pipeline")
def pipeline():
	PipeSim()

	cycles["0"] = 0
	with open("./inst_pipe.txt", "r") as f:
		for line in f:
			foo = line.split(":");	
			instruction_index[foo[0]] = foo[1];
	with open("./branch.txt", "r") as f:
		for line in f:
			foo = line.split(":");
			branch_details[foo[0]] = [foo[1], foo[2], foo[3][:-1]];
	return render_template("pipeline.html",
				pipeline=pipeline_data,
				instructions=instruction_index,
				cycount=cycles["0"],
				branched=branch_details,
				ran=run_status,
				memory=memory,
				regfile=RF,
				chazards=chazards,
				dhazards=dhazards)


@app.route("/pipeline/program", methods=["GET", "POST"])
def pipeline_program():
	new = out.readline()
	new2=out2.readline()
	run_status = 0;
	if len(new2):
		if("done" in request.form):
			while(True):
				new2 = out2.readline().rstrip("\n")
				if(len(new2)==0):
					break
				if new2[0] == "X":
					new2 = new2.split(":")
					RF[new2[0]] = new2[1]
					new2 = out2.readline().rstrip("\n")
				if new2[0] == "-":
					new2 = out2.readline()
					if new2[0] != '\n':
						memory.clear()
						while(new2[0] != "\n"):
							new2=new2.split(":")
							data = new2[1].split(" ")
							memory[new2[0]] = [data[0], data[1], data[2], data[3]]
							new2=out2.readline()
		else:
			new2 = out2.readline().rstrip("\n")
			if new2[0] == "X":
				new2 = new2.split(":")
				RF[new2[0]] = new2[1]
				new2 = out2.readline().rstrip("\n")
			if new2[0] == "-":
				new2 = out2.readline()
				if new2[0] != '\n':
					memory.clear()
					while(new2[0] != "\n"):
						new2=new2.split(":")
						data = new2[1].split(" ")
						memory[new2[0]] = [data[0], data[1], data[2], data[3]]
						new2=out2.readline()
						
	if len(new):
		if("done" in request.form):
			while(len(new)):
				cycles["0"] += 1
				lol=[]
				for i in range(5):
					new = out.readline().rstrip("\n")
					new = new.split(":")
					if(new[1] == ""):
						lol.append("")	
					else:
						lol.append(new[1])
				new = out.readline().rstrip("\n")		
				pipeline_data[new] = lol
				if((lol[2] == lol[3]) and (lol[2] == "")):
						if(new!='1'):
							if(lol[4] != lol[2]):
								chazards[new] = '1'
				if(lol[2] == "" and lol[1] != lol[2] and lol[3] != lol[2]):
					dhazards[new] = '1'
				new = out.readline()
			run_status = 1;
		else:
			cycles["0"] += 1
			lol=[]
			for i in range(5):
				new = out.readline().rstrip("\n")
				new = new.split(":")
				if(new[1] == ""):
					lol.append("")	
				else:
					lol.append(new[1])
			new = out.readline().rstrip("\n")		
			pipeline_data[new] = lol
			if((lol[2] == lol[3]) and (lol[2] == "")):
						if(new!='1'):
							if(lol[4] != lol[2]):
								chazards[new] = '1'
			if(lol[2] == "" and lol[1] != lol[2] and lol[3] != lol[2]):
					dhazards[new] = '1'
		next = cycles["0"]
	else:
		next = cycles["0"];
		run_status = 1;
		try:
			os.system("rm ./OUTPUT.txt ./instructions.txt ./data_out.mc")
		except:
			pass

	return render_template('pipeline.html', 
				pipeline=pipeline_data,
				instructions=instruction_index,
				cycount=next,
				ran=run_status,
				branched=branch_details,
				memory=memory,
				regfile=RF,
				chazards=chazards,
				dhazards=dhazards)



@app.route("/single")
def single():
	SingleSim()

	instructions.clear()
	memory.clear()
	RF = RFinit
	cycles["0"] = 0
	cycles["max"] = 0
	with open("./instructions.txt", "r") as f:
		for line in f:
			foo = line.split(":")
			instructions[foo[0]] = [foo[1],foo[2],foo[3]]
			cycles["max"] += 1
	return render_template('single.html', 
				regfile=RF, 
				instructions=instructions,
				memory=memory,
				cycount=cycles["0"])


@app.route("/single/program", methods=["GET", "POST"])
def single_program():
	if(cycles["0"] < cycles["max"]):
		if("done" in request.form):
			loop = cycles["max"] - cycles["0"]
		else:
			loop = 1
		while(loop>0):
			cycles["0"] += 1
		
			inst = out.readline()
			lil = list(instructions.items())
			instructions.pop(lil[0][0])
	
			new = out.readline()
			while(new[0] == "X"):
				new = new.split(":")
				RF[new[0]] = new[1]
				new = out.readline()
	
			new = out.readline()
			if new[0] != '\n':
				memory.clear()
				while(new[0] != "\n"):
					new=new.split(":")
					data = new[1].split(" ")
					memory[new[0]] = [data[0], data[1], data[2], data[3]]
					new=out.readline()
			loop-=1	
		next = cycles["0"]
	else:
		next = cycles["0"]
		try:
			os.system("rm ./OUTPUT.txt ./instructions.txt ./data_out.mc")
		except:
			pass
	return render_template('single.html', 
				regfile=RF, 
				instructions=instructions,
				memory=memory,
				cycount=next)

if __name__ == "__main__":
	app.run(debug=True)