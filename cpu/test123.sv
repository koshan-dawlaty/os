package cfg;
    localparam int NUM_REGISTERS = 32;
    localparam int REGISTER_SEL_BITS = $clog2(NUM_REGISTERS);
    localparam int REGISTER_WIDTH = 32;
    localparam int ADDRESS_WIDTH = 32;
endpackage

typedef logic [cfg::REGISTER_SEL_BITS - 1:0] reg_num_t;
typedef logic [cfg::REGISTER_WIDTH - 1:0] reg_t;
typedef logic [cfg::ADDRESS_WIDTH - 1:0] addr_t;

interface memory_bus(input clk);
	addr_t addr;
	addr_t data_in;

	/* maybe should be wire? it would not work with testbenches */
	logic is_writing;
	logic is_enabled = 1;

endinterface

module memory_sim (
	memory_bus bus,
    output addr_t data_out

);

    // addr_t mem[0:1023] = {31'h10841, 31'h21041};
    addr_t mem[0:1023];

    initial begin
        $readmemh("program.hex", mem);
        /*           ........r3...r2...r1...op...i */
        //    				  01000011000010000100
        /*          imm...........r2...r1...op...i */
        // 000000000100000000001000001
        //        00100001000001000001
        //
        //     1st 1000000000001000010
        //     2nd 0100000100001000001
        //
        //
        /*          imm...........r2...r1...op...i */
        //        00110001000001000001

        // 00010001000001000001
        // mem[0] = 31'b00000000001010000100001000011;
        // mem[1] = 31'b00000000001010001000010000011;
        // mem[2] = 31'b00000000001010001100011000011;
        /*           ........r3...r2...r1...op...i */
        // mem[3] = 31'b00000000000010001000001000000;
        // mem[4] = 31'b00000000000010001100001000000;


        // mem[0] = 31'h10841;
        // mem[1] = 31'h21041;
        // mem[2] = 31'h42;
    end


    addr_t temp_addr = 0;

    always @(posedge bus.clk) begin
        if (bus.is_writing && bus.is_enabled) begin
            mem[bus.addr] <= bus.data_in;
        end else begin
            temp_addr <= bus.addr;
        end
    end

    assign data_out = mem[temp_addr];

endmodule

module test_memory_sim ();

    logic  clk;
	memory_bus bus (
		.clk(clk)
	);

    addr_t data_out;

    memory_sim memory_sim (.*);

    initial begin
        clk = 0;

        bus.is_enabled = 1;

        bus.is_writing = 1;
        bus.addr = 4;
        bus.data_in = 123;
        clk = ~clk;
        #1;
        clk = ~clk;
        #1;

        bus.is_writing = 0;
        bus.addr = 4;
        clk = ~clk;
        #1;
        clk = ~clk;
        #1;
        assert (data_out == 123);

        $display("test_memory_sim ok");

    end

endmodule



module add1 (
    input  wire one,
    input  wire two,
    input  wire carry_in,
    output wire out,
    output wire carry
);
    assign out   = one ^ two ^ carry_in;
    assign carry = (one & two) | (one & carry_in) | (two & carry_in);
endmodule

module test_add1;

    reg  one;
    reg  two;
    reg  carry_in;

    wire out;
    wire carry_out;

    add1 test (
        one,
        two,
        carry_in,
        out,
        carry_out
    );

    initial begin
        one = 1;
        two = 0;
        carry_in = 0;
        #1 assert (out == 1 && carry_out == 0);
        $display("test_add1 ok");
    end


endmodule


module registers (
    input reg_num_t reg_num,
    input wire should_write,
    input wire clk,
    input reg_t data_in,
    output reg_t data_out
);


    reg_t reg_list[cfg::NUM_REGISTERS];

    initial begin
        for (int i = 0; i < cfg::NUM_REGISTERS; i++) reg_list[i] = 0;
    end


    always @(posedge clk) begin

        if (should_write) begin
            reg_list[reg_num] <= data_in;
        end
    end

    assign data_out = reg_list[reg_num];
endmodule

module test_registers;
    reg_num_t reg_num;
    logic should_write;
    logic clk;
    reg_t data_in;
    reg_t data_out;

    registers registers (.*);

    initial begin
        /* FIXME: downcast */
        for (reg_num_t i = 0; i < cfg::NUM_REGISTERS - 1; i++) begin
            clk = 0;
            #1;
            should_write = 1;
            reg_num = i;
            data_in = 123;
            clk = ~clk;
            #1;
            clk = ~clk;
            #1;
            should_write = 0;
            reg_num = i;
            clk = ~clk;
            #1;
            assert (data_out == 123);
        end
        $display("test_registers ok");
    end

endmodule

module opcode_handle (
    input wire reg_t reg1,
    input wire reg_t reg2,
    input wire [4:0] opcode,
    input wire is_enabled,

    output reg_t out
);
    always_comb begin : opcode_select
        out = 0;
        if (is_enabled) begin
            case (opcode)
                0: out = reg1 + reg2;
                1: out = reg1;
                2: out = reg1 - reg2;
                3: out = reg1 * reg2;
                31: $finish();

				/* these are for memory instructions so ignore them. */	
				16: ; 
				24: ;

                default: $error("no case for %d", opcode);
            endcase
        end
    end

endmodule

module immediate_opcode_handle (
    input logic [4:0] opcode,
    input reg_t register,
    input logic [15:0] immediate,

    input wire is_enabled,

    output reg_t out
);
    logic [15:0] reg_16;
    assign reg_16 = register[15:0];

    always_comb begin : opcode_select
        out = 0;
        if (is_enabled) begin
            case (opcode)
                0: begin
                    out = reg_16 + immediate;
                end
                1:  out[15:0] = immediate;
                2:  out = reg_16 - immediate;
                3:  out = reg_16 * immediate;
                31: $finish();

				/* FIXME: this is probably not good */
				/* these are for memory instructions so ignore them. */	
				16: ; 
				24: ;

                default: $error("no immediate case for %d", opcode);
            endcase
        end
    end
endmodule

module run_tests;
    test_add1 test_add1 ();
    test_registers test_registers ();
    test_memory_sim test_memory_sim ();
endmodule

module decode_instruction (
    input wire [31:0] instruction,
    output wire reg_num_t reg1_num,
    output wire reg_num_t reg2_num,
    output wire reg_num_t reg_out_num,
    output wire [4:0] opcode
);
    assign opcode = instruction[5:1];
    assign reg1_num = instruction[10:6];
    assign reg2_num = instruction[15:11];
    assign reg_out_num = instruction[20:16];
endmodule

typedef enum logic [4:0] {
    INSTRUCTION_FETCH_1 = 0,
    INSTRUCTION_FETCH_2,
    INSTRUCTION_FETCH_3,

    INSTRUCTION_SELECT,

    NORMAL_REG_1,
    NORMAL_REG_2,
    NORMAL_WRITE,

    IMMEDIATE_REG_1,
    IMMEDIATE_WRITE,

    MEMORY_LOAD_START,
    MEMORY_LOAD_SKIP,
    MEMORY_LOAD_WRITE,

    MEMORY_STORE_START,
    MEMORY_STORE_WRITE
} instruction_state_t;

module test123;

    reg_num_t reg_num;
    logic should_write;
    logic clk = 0;
    reg_t data_in;
    wire reg_t data_out;
    registers registers (.*);


    reg_t reg1;
    reg_t reg2;
    reg [31:0] instruction = 0;
    wire reg_num_t reg1_num;
    wire reg_num_t reg2_num;
    wire reg_num_t reg_out_num;
    wire [4:0] opcode;

    decode_instruction decode_instruction (.*);

    reg_t reg_out;

    wire  immediate_opcode_enabled;
    assign immediate_opcode_enabled = instruction[0] == 1;

    opcode_handle opcode_handle (
        .*,
        .is_enabled(!immediate_opcode_enabled),
        .out(reg_out)
    );


    reg_t immediate_register;
    logic [15:0] immediate_value;
    reg_t immediate_out;


    immediate_opcode_handle immediate_opcode_handle (
        .opcode    (opcode),
        .register  (immediate_register),
        .immediate (immediate_value),
        .is_enabled(immediate_opcode_enabled),
        .out       (immediate_out)
    );

    logic [31:0] current_instruction = 0;

	memory_bus mem_bus (clk);
    wire addr_t mem_out;

    memory_sim memory_sim (
		.bus(mem_bus),
        .data_out  (mem_out)
    );

    reg_t store_address;


    assign immediate_value = instruction[31:16];

    initial begin
        $dumpfile("wave.vcd");
        $dumpvars(0, test123);
        forever begin
            #1 clk = ~clk;
        end
    end

    instruction_state_t state = INSTRUCTION_FETCH_1;

    always_ff @(posedge clk) begin
        case (state)
            INSTRUCTION_FETCH_1: begin
                mem_bus.addr <= current_instruction;
                mem_bus.is_writing <= 0;
                state <= INSTRUCTION_FETCH_2;
            end
            INSTRUCTION_FETCH_2: begin
                /* FIXME: this is stupid, it is just here to skip a clock
				* cycle as memory needs to wait one cycle to take a new
				* address, this could also be somewhat avoided by setting the address at the end of decoding an instruction */
                state <= INSTRUCTION_FETCH_3;
            end
            INSTRUCTION_FETCH_3: begin
                instruction <= mem_out;
                current_instruction <= current_instruction + 1;
                state <= INSTRUCTION_SELECT;
            end
            INSTRUCTION_SELECT: begin
                // $display("instruction is %x, next instruction: %d", instruction,
                //          current_instruction);


                reg_num <= reg1_num;
                should_write <= 0;
                if (instruction[0] == 0) begin
                    state <= NORMAL_REG_1;
                end else begin
                    if (opcode[4] == 1) begin
                        if (opcode[3] == 0) begin
							$display("loading from reg num %d", reg1_num);
                            state <= MEMORY_LOAD_START;
                        end else begin
							$display("storing at reg num %d", reg1_num);
                            state <= MEMORY_STORE_START;
                        end
                    end else begin
                        $display("have immediate:%d, reg:%d instr is %x", immediate_value,
                                 reg1_num, instruction);
                        state <= IMMEDIATE_REG_1;
                    end
                end
            end

            IMMEDIATE_REG_1: begin
                immediate_register <= data_out;
                state <= IMMEDIATE_WRITE;
            end
            IMMEDIATE_WRITE: begin
                $display("writing %d to %d", immediate_out, reg2_num);
                data_in <= immediate_out;
                reg_num <= reg2_num;
                should_write <= 1;

                state <= INSTRUCTION_FETCH_1;
            end

            NORMAL_REG_1: begin
                reg1 <= data_out;

                should_write <= 0;
                reg_num <= reg2_num;

                state <= NORMAL_REG_2;
            end
            NORMAL_REG_2: begin
                reg2  <= data_out;

                state <= NORMAL_WRITE;
            end
            NORMAL_WRITE: begin

                should_write <= 1;
                reg_num <= reg_out_num;
                data_in <= reg_out;

                $display("normal: writing %d to %d using %d and %d", reg_out, reg_out_num,
                         reg1_num, reg2_num);

                state <= INSTRUCTION_FETCH_1;
            end

            MEMORY_LOAD_START: begin
				$display("load start at addr %d", data_out);
                mem_bus.addr <= data_out;
                mem_bus.is_writing <= 0;

                state <= MEMORY_LOAD_SKIP;
            end
			MEMORY_LOAD_SKIP: begin
				state <= MEMORY_LOAD_WRITE;	
			end
            MEMORY_LOAD_WRITE: begin
                data_in <= mem_out;
                reg_num <= reg2_num;
                should_write <= 1;

				$display("read %d after load", mem_out);

                state <= INSTRUCTION_FETCH_1;
            end

            MEMORY_STORE_START: begin
				$display("store start, address is %d", data_out);
                store_address <= data_out;
                reg_num <= reg2_num;
                should_write <= 0;
                state <= MEMORY_STORE_WRITE;
            end
            MEMORY_STORE_WRITE: begin
                mem_bus.addr <= store_address;
                mem_bus.data_in <= data_out;
                mem_bus.is_writing <= 1;

				$display("storing value %d", data_out);

                state <= INSTRUCTION_FETCH_1;
            end


            default: $error("invalid instruction phase %d", current_instruction);
        endcase
    end


endmodule
