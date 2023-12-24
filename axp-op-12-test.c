/*
 * AXP opcode 12 test
 *
 * Copyright (c) 2021-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdio.h>

static inline uint64_t axp_srl (uint64_t a, uint64_t b)
{
	return a >> (b & 63);
}

static inline uint64_t axp_sll (uint64_t a, uint64_t b)
{
	return a << (b & 63);
}

static inline uint64_t axp_sra (int64_t a, uint64_t b)
{
	return a >> (b & 63);
}

static uint64_t axp_zap (uint64_t x, unsigned mask)
{
	uint64_t m, bm;
	int i;

	for (m = 0, bm = 0xff, i = 1; i < 0x100; bm <<= 8, i <<= 1)
		if ((mask & i) != 0)
			m |= bm;

	return x & ~m;
}

static unsigned axp_byte_mask (int f)
{
	switch (f & 0x30) {
	case 0x00:  return 0x0001;
	case 0x10:  return 0x0003;
	case 0x20:  return 0x000f;
	default:    return 0x00ff;
	}
}

uint64_t axp_msk_0 (int f, uint64_t a, uint64_t b)
{
#if 0
	unsigned bs = b & 7;
	unsigned m  = axp_byte_mask (f) << bs;
	unsigned sm = (f & 0x40) ? m >> 8 : m;
#else
	unsigned m  = axp_byte_mask (f);
	unsigned bs = b & 7;
	unsigned bn = (f & 0x40) ? 8 - bs      : bs;
	unsigned sm = (f & 0x40) ? m >> bn     : m << bn;
#endif
	return axp_zap (a, sm);
}

uint64_t axp_ins_0 (int f, uint64_t a, uint64_t b)
{
#if 0
	unsigned bs = b & 7;
	unsigned m  = axp_byte_mask (f) << bs;
	unsigned sm = (f & 0x40) ? m >> 8      : m;
	unsigned bl = (f & 0x40) ? 64 - bs * 8 : bs * 8;
	uint64_t as = (f & 0x40) ? a >> bl     : a << bl;
#else
	unsigned m  = axp_byte_mask (f);
	unsigned bs = b & 7;
	unsigned bn = (f & 0x40) ? 8 - bs      : bs;
	unsigned sm = (f & 0x40) ? m >> bn     : m << bn;
	uint64_t as = (f & 0x40) ? a >> bn * 8 : a << bn * 8;
#endif
	return axp_zap (as, ~sm);
}

uint64_t axp_ext_0 (int f, uint64_t a, uint64_t b)
{
	unsigned m  = axp_byte_mask (f);
	unsigned bs = b & 7;
#if 0
	unsigned bl = (f & 0x40) ? 64 - bs * 8 : bs * 8;
	uint64_t as = (f & 0x40) ? a << bl     : a >> bl;
#else
	unsigned bn = (f & 0x40) ? 8 - bs      : bs;
	uint64_t as = (f & 0x40) ? a << bn * 8 : a >> bn * 8;
#endif
	return axp_zap (as, ~m);
}

static inline uint64_t axp_sr (int f, uint64_t a, uint64_t b)
{
	switch (f & 0x0c) {
	case 0x00:  return a;
	case 0x04:  return axp_srl (a,  b);
	case 0x08:  return axp_sll (a, -b);  /* srl/sra overflow word */
	default:    return axp_sra (a,  b);
	}
}

static inline uint64_t axp_ins_pm (int f, uint64_t a, uint64_t b)
{
	return axp_sr (f, axp_zap (a, ~axp_byte_mask (f)), b * 8);
}

static
uint64_t axp_bop (int f, uint64_t a, uint64_t b, uint64_t bm, int y, int z)
{
	unsigned m  = axp_byte_mask (f);
//	unsigned sm = (f & 0x48) == 0x40 ? m >> (-bm & 7) : m << (bm & 7);
	unsigned sm = (f & 0x48) == 0x40 ? m << (bm & 7) >> 8 : m << (bm & 7);
	uint64_t as = axp_sr (f, a, (f & 2) ? b * 8 : b);

	unsigned ms = y ? sm : m;

	return axp_zap (as, z ? ~ms : ms);
}

static inline uint64_t axp_msk (int f, uint64_t a, uint64_t b)
{
	return axp_bop (f, a, b, b, 1, 0);
}

static inline uint64_t axp_msk_1 (int f, uint64_t a, uint64_t b, int x, int y)
{
	unsigned m  = axp_byte_mask (f);
	unsigned mn = x ? m ^ 0x00ff : m;  /* invert whole 8-bit mask */
	unsigned sm = (f & 0x48) == 0x40 ? mn << (b & 7) >> 8 : mn << (b & 7);

	return axp_zap (a, y ? ~sm : sm);
}

static inline uint64_t axp_ins (int f, uint64_t a, uint64_t b, uint64_t bm)
{
	return axp_bop (f, a, b, bm, 1, 1);
}

static inline uint64_t axp_ext (int f, uint64_t a, uint64_t b, uint64_t bm)
{
	return axp_bop (f, a, b, bm, 0, 1);
}

static uint64_t axp_shift (int f, uint64_t a, uint64_t b, uint64_t c)
{
	switch (f & 0x0f) {
	case 0x00:  return axp_zap (   a,  b);
	case 0x01:  return axp_zap (   a, ~b);
	case 0x02:  return axp_msk_1 (f, a,  b, 0, 0);
	case 0x03:  return axp_msk_1 (f, a,  b, 1, 1);
	}

	switch (f & 0x4f) {
	case 0x04:  return axp_sr  (f, a,  b    );		// srl
	case 0x05:  return axp_sr  (f, a, -b    );		// sll.high
	case 0x06:  return axp_ext (f, a,  b    ,  b    );	// extl
	case 0x07:  return axp_ins (f, a, -b    , -b    );

	case 0x08:  return axp_sr  (f, a,  b    );		// srl.frac
	case 0x09:  return axp_sr  (f, a, -b    );		// sll
	case 0x0a:  return axp_ext (f, a,  b    , -b    );
	case 0x0b:  return axp_ins (f, a, -b    ,  b    );	// insl

	case 0x0c:  return axp_sr  (f, a,  b    );		// sra
	case 0x0d:  return axp_sr  (f, a, -b    );
	case 0x0e:  return axp_ext (f, a,  b    ,  b    );
	case 0x0f:  return axp_ins (f, a, -b    ,  b    );

	case 0x44:  return axp_sr  (f, a,  b + 1);
	case 0x45:  return axp_sr  (f, a, -b    );
//	case 0x46:  return axp_ext (f, a,  b    ,  b    );	// wrong mask
	case 0x47:  return axp_ins (f, a, -b    ,  b    );	// insh

	case 0x48:  return axp_sr  (f, a,  b    );
	case 0x49:  return axp_sr  (f, a, ~b    );
	case 0x4a:  return axp_ext (f, a,  b    ,  b    );	// exth
	case 0x4b:  return axp_ins (f, a, -b    ,  b    );

	case 0x4c:  return axp_sr  (f, a,  b);
	case 0x4d:  return axp_sr  (f, a, ~b);
	case 0x4e:  return axp_ext (f, a,  b    ,  b    );
	case 0x4f:  return axp_ins (f, a, -b    ,  b    );	// 7F broken, mask = 0
	}

	return c;
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
		c = axp_zap (s, ~m);

		c = axp_shift (op, a, b, c);

//		printf ("%2X %016lx %02lx\n", op, c, m & 0xff);
		printf ("%2X %016lx\n", op, c);
	}

	return 0;
}
