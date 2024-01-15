/*
 * AXP opcode 11 test
 *
 * Copyright (c) 2021-2024 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdio.h>

#define F(x)	((f >> (x)) & 1)

#define F0	F (0)
#define F1	F (1)
#define F2	F (2)
#define F3	F (3)
#define F4	F (4)
#define F5	F (5)
#define F6	F (6)

static inline int axp_cond (int lbs, int eq, int lt, int inv, uint64_t a)
{
	const int e = eq & (a == 0);
	const int l = lt & ((a >> 63) & 1);
	const int c = lbs ? (a & 1) : (e | l);

	return inv ^ c;
}

/* opcode 11, bitwise operations and conditional move */

static inline uint64_t axp_logic (int f, uint64_t a, uint64_t b, uint64_t c)
{
	const uint64_t amask = 1;			/* bwx		*/
	const uint64_t impl  = 1;			/* 21164*	*/

	uint64_t bb  = F3 ? ~b : b;			/* 08 - invert	*/

	uint64_t bitop = F6 ? a ^ bb :			/* 40 - xor	*/
			 F5 ? a | bb : a & bb;		/* 20 - or	*/

	const int lbs = F4;				/* 10 - lbs	*/
	const int eq  = F5;				/* 20 - eq	*/
	const int lt  = F6;				/* 40 - lt	*/
	const int inv = F1 & (F2 | !F4);
	const int cond = axp_cond (lbs, eq, lt, inv, a);

	int cmov = F2 | F1;				/* 02/04 - cmov	*/

	uint64_t r  = cmov ? b : bitop;
	uint64_t rm = (f & 0x61) == 0x61 ? r & ~amask : r;
	uint64_t cb = F3 ? impl : c;			/* 08 - implver	*/

	return (cond | !cmov) ? rm : cb;
}

int main (int argc, char *argv[])
{
	const uint64_t a = 0x0fedcba987654321ull, b = 3, c = 1;
	unsigned f;
	uint64_t r;

	printf ("# a = %016lx, b = %ld\n\n", a, b);

	for (f = 0; f < 0x80; ++f) {
		r = axp_logic (f, a, b, c);

		printf ("%2X %016lx\n", f, r);
	}

	return 0;
}
