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

/* opcode 11, bitwise operations and conditional move */

module axp_logic #(
	parameter AMASK = 1				/* bwx		*/
	parameter IMPL  = 1				/* 21164	*/
)(
	input  [31:0] cmd, input [63:0] a, input [63:0] b, input [63:0] c,
	output [63:0] y
);
	wire [6:0] f = cmd[11:5];

	wire [63:0] bb = f[3] ? ~b : b;			/* 08 - invert	*/

	wire [63:0] bitop = f[6] ? a ^ bb :		/* 40 - xor	*/
			    f[5] ? a | bb : a & bb;	/* 20 - or	*/

	wire lt   = f[6] & a[63];			/* 40 - lt	*/
	wire eq   = f[5] & ~|a;				/* 20 - eq	*/
	wire lbs  = f[4] ? a[0] : eq | lt;		/* 10 - lbs	*/

	wire inv  = f[1] & (f[2] | ~f[4]);
	wire cmov = f[2] | f[1];			/* 02/04 - cmov	*/
	wire cond = (inv ^ lbs) | ~cmov;

	wire [63:0] r  = cmov ? b : bitop;
	wire [63:0] rm = (f[6] & f[5] & f[0]) ? r & ~AMASK : r;
	wire [63:0] cb = f[3] ? IMPL : c;		/* 08 - implver	*/

	assign y = cond ? rm : cb;
endmodule

/* opcode 12, byte manipulation and shift */

module axp_shift (
	input [6:0] f, input [63:0] x, input [63:0] n, output [63:0] y
);
	wire [5:0] ns = f[1] ? n[8:3] : n[5:0];		/* scale	*/
	wire [5:0] nn = f[0] ? -ns : ns;		/* negate	*/

	wire signed [127:0] sx = x << 64;
	wire signed [127:0] sy = f[3:2] == 3 ? sx >>> nn : sx >> nn;

	assign y = f[2] ? sy[127:64] : f[3] ? sy[63:0] : x;
endmodule

module axp_mask (
	input [6:0] f, input [63:0] x, output [63:0] y
);
	wire [15:0] m  = {{4 {f[5] & f[4]}}, {2 {f[5]}}, (f[5] | f[4]), 1'b1};
	wire        q0 = (f[3:2] == 0);
	wire [15:0] ms = (q0 | f[0]) ? m << x[2:0] : m;	/* msk or ins	*/
	wire [15:0] mh = f[6] & ~f[3] ? ms >> 8 : ms;	/* high part	*/
	wire [7:0]  mf = f[1] ? mh :			/* byte ops	*/
			 q0   ?  x : 8'hff;		/* zap* / shift	*/
//	assign y = (f[3:0] == 0 || f[3:1] == 1) ? ~mf : mf;
	assign y = (q0 && !f[0]) ? ~mf : mf;		/* zap or msk?	*/
endmodule

module axp_zapnot (
	input [63:0] x, input [63:0] mask, output [63:0] y
);
	genvar i;

	for (i = 0; i < 64; i = i + 1)
		assign y[i] = x[i] & mask[i >> 3];
endmodule

module axp_shifter (
	input  [31:0] cmd, input [63:0] a, input [63:0] b,
	output [63:0] y
);
	wire [6:0] f = cmd[11:5];
	wire [63:0] m, s;

	axp_shift  S (f, a, b, s);
	axp_mask   M (f, b, m);
	axp_zapnot Z (s, m, y);
endmodule

`endif  /* AXP_OP_V */
