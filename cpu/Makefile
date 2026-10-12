BIN=bin

VVP_FILE=$(BIN)/test123.vvp
IN_FILES=test123.sv

run:
	iverilog -g2012 -o $(VVP_FILE) $(IN_FILES) -s test123
	vvp $(VVP_FILE)

test:
	iverilog -g2012 -o $(VVP_FILE) $(IN_FILES) -s run_tests
	vvp $(VVP_FILE)

