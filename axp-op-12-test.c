/*
 * AXP opcode 12 test
 *
 * Copyright (c) 2021-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdio.h>

uint64_t zapnot (uint64_t x, unsigned mask)
{
	size_t i;
	uint64_t m, bm;

	for (i = 0, m = 0, bm = 0xff; i < 8; ++i, bm <<= 8)
		if ((mask & (1 << i)) != 0)
			m |= bm;

	return x & m;
}

static inline unsigned make_mask_v1 (int f)
{
	unsigned m = 0x01;

	m |= (f & 0x30) ? 0x03 : 0;
	m |= (f & 0x20) ? 0x0f : 0;
	m |= ((f & 0x30) == 0x30) ? 0xff : 0;

	return m;
}

static inline unsigned get_mask (int f, uint64_t b)
{
	if ((f & 0xcf) == 0x46) f = 0x36;	/* WTF?			*/

	const unsigned order = (f & 0x30) >> 4;
	const unsigned count = 1u << order;
	int m = ~(~0 << count);

	if ((f & 0xc) == 0 || (f & 1))		/* msk or ins		*/
		if (f != 0x7f) {		/* WTF?			*/
			m = m << (b & 7);	/* to fix x3: cin = 1?	*/

			if ((f & 7) == 3)		/* WTF?		*/
				m |= (1 << (b & 7)) - 1;
		}

	int mh = ((f & 0x48) == 0x40) ? m >> 8 : m;	/* high part	*/

	if ((f & 0x4f) == 0x43)				/* WTF?		*/
		mh = ~((1 << (b & 7)) - 1);

	m = (f & 0x2)      ? mh :		/* byte ops		*/
	    (f & 0xc) == 0 ?  b : ~0;		/* zap* or shift	*/
						/* zap or msk? invert	*/
	return (f & 0xf) == 0 || (f & 0xe) == 2 ? ~m : m;
}

static inline unsigned get_mask_v2 (int f, uint64_t b)
{
	const unsigned order = (f & 0x30) >> 4;
	const unsigned count = 1u << order;

	unsigned shift = ((f & 0xc) == 0 || (f & 1)) &&	/* msk or ins	*/
			 f != 0x7f ? b & 7 : 0;

	int mb = (1 << (shift + 0    )) - 1;
	int mt = (1 << (shift + count)) - 1;

	if ((f & 0x4f) == 0x03 || f == 0x73) mb =  0;	/* extra	*/
	if ((f & 0x4f) == 0x43 && f != 0x73) mt = ~0;	/* extra	*/

	if ((f & 0x02) == 0)		     mb =  0;	/* shift ops	*/
	if ((f & 0x02) == 0)		     mt = ~0;	/* shift ops	*/

	int mc = mt & ~mb;

	int mh = (f & 0x48) == 0x40 &&
		 (f & 7) != 3 ?				/* extra	*/
		mc >> 8 : mc;				/* high part	*/

	int m = ((f & 0x0e) == 0) ? b : mh;		/* zap* direct	*/
						/* zap or msk? invert	*/
	return (f & 0xf) == 0 || (f & 0xe) == 2 ? ~m : m;
}

static inline uint64_t shift (int f, uint64_t x, unsigned n)
{
	static const
//			 -  -  -  -   1  1  0  1   0  0  0  1   0  0  0  1
	char cmap[16] = {1, 1, 0, 1,  1, 1, 0, 1,  0, 0, 0, 1,  0, 0, 0, 1};

	n  = (f & 0x02) ? n * 8 : n;
	n  = (f & 0x01) ? ~n : n;
//	n += (f & 0x01);	/* or just negate: should be ok for ISA ops */
	n += (f & 0x40) ? cmap [f & 0x0f] : (f & 0x01);	/* cmap for extra ops */

	switch (f & 0x0c) {
	case 0x00:
		return x;			/* n = 0		*/
	case 0x04:
		return x >> (n & 63);		/* srl			*/
	case 0x08:
		return x << (-n & 63);		/* srl overflow word	*/
	default:
		return (int64_t) x >> (n & 63);	/* sra			*/
	}
}

int main (int argc, char *argv[])
{
//	const uint64_t a = 0x0fedcba987654321ull, b = 3;
	const uint64_t a = 0x8fedcba987654321ull, b = 11;
	uint64_t s, m, c;
	unsigned op;

	printf ("# a = %016lx, b = %ld\n\n", a, b);

	for (op = 0; op < 0x80; ++op) {
		s = shift (op, a, b);
		m = get_mask (op, b);
		c = zapnot (s, m);

//		printf ("%2X %016lx %02lx\n", op, c, m & 0xff);
		printf ("%2X %016lx\n", op, c);
	}

	return 0;
}
