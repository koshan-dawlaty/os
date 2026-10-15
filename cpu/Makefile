BIN=bin

VVP_FILE=$(BIN)/test123.vvp
IN_FILES=test123.sv

run:
	verilator --binary -Wno-WIDTH -j 0 --top-module test123 test123.sv
	./obj_dir/Vtest123

test:
	iverilog -g2012 -o $(VVP_FILE) $(IN_FILES) -s run_tests
	vvp $(VVP_FILE)

