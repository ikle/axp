/*
 * AXP Operations
 *
 * Copyright (c) 2021-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

`ifndef AXP_OP_V
`define AXP_OP_V  1

/* opcode 10, adder and comparator (w/o cmpbge) */

module axp_adder (
	input  [31:0] cmd, input [63:0] a, input [63:0] b,
	output [63:0] y
);
	wire [6:0] f = cmd[11:5];

	wire cin        = f[0];				/* 01 - carry	*/
	wire [63:0] as  = f[4] ? (a  << 1) : a;		/* 10 - shift 1	*/
	wire [63:0] ass = f[1] ? (as << 2) : a;		/* 02 - shift 2	*/
	wire [63:0] bb  = f[3] ? ~b : b;		/* 08 - invert	*/

	wire [64:0] s   = (ass + bb) + cin;		/* 20 - !sext	*/
	wire [63:0] ss  = f[5] ? s[63:0] : {{32 {s[31]}}, s[31:0]};

	wire lt  = f[6] & s[63];			/* 40 - lt	*/
	wire eq  = f[5] & ~|s;				/* 20 - eq	*/
	wire ult = f[4] & ~s[64];			/* 10 - ult	*/

	assign y = f[2] ? (ult | eq | lt) : ss;		/* 04 - cmp	*/
endmodule

/* opcode 11, bitwise operations and conditional move (w/o amask/implver) */

module axp_logic (
	input  [31:0] cmd, input [63:0] a, input [63:0] b, input [63:0] c,
	output [63:0] y
);
	wire [6:0] f = cmd[11:5];

	wire [63:0] bb = f[3] ? ~b : b;			/* 08 - invert	*/

	/* in xor branch: add amask (x61) and implver (x6c) */

	wire [63:0] bitop = f[6] ? a ^ bb :		/* 40 - xor	*/
			    f[5] ? a | bb : a & bb;	/* 20 - or	*/

	wire lt   = f[6] & a[63];			/* 40 - lt	*/
	wire eq   = f[5] & ~|a;				/* 20 - eq	*/
	wire lbs  = f[4] ? a[0] : eq | lt;		/* 10 - lbs	*/

	wire inv  = f[1] & (f[2] | ~f[4]);
	wire cond = inv ^ lbs;

	wire [63:0] cmov = cond ? b : c;

	assign y = (f[2] | f[1]) ? cmov : bitop;	/* 02/04 - cmov	*/
endmodule

`endif  /* AXP_OP_V */
