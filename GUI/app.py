import os
import webbrowser
from flask import Flask, render_template, request

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

memory={}
cycles={"0":0, "max":0}
inst_no=0

def runSim():
	os.system("cd ../src && make")
	os.system("../bin/myRISCVSim ../test/input.mc")

@app.route("/")
def home():
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
	return render_template('home.html', 
				regfile=RF, 
				instructions=instructions,
				memory=memory,
				cycount=cycles["0"])

@app.route("/program", methods=["GET", "POST"])
def program():
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
	return render_template('home.html', 
				regfile=RF, 
				instructions=instructions,
				memory=memory,
				cycount=next)

if __name__ == "__main__":
	runSim()
	out = open("./OUTPUT.txt", "r")
	app.run()