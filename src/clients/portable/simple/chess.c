/*                        Copyright (c) 1988 Bellcore
 *                            All Rights Reserved
 *       Permission is granted to copy or use this program, EXCEPT that it
 *       may not be sold for profit, the copyright notice must be reproduced
 *       on copies, and credit should be given to Bellcore where it is due.
 *       BELLCORE MAKES NO WARRANTY AND ACCEPTS NO LIABILITY FOR THIS PROGRAM.
 */

/* MGR interface to /usr/games/chess by S. D. Hawley */

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <mgr/mgr.h>

/* geometry constants */

#define MAPW 96
#define MAPH 224
#define MAPNUM 1
#define SCRATCH 2
#define PIECEW 32
#define PIECEH 32
#define BOARDSIZE 64
#define FONTWIDTH 12	/* guess at the font width */
#define WIN_W (9*PIECEW+16 + (14 * FONTWIDTH))
#define WIN_H (9*PIECEW+16)
#define MOVESX (8 * PIECEW + 16)
#define MOVESY 12
#define MOVESW 14 * FONTWIDTH
#define MOVESH (WIN_H - 40)

/* other */

#ifndef B_COPY
#define B_COPY BIT_SRC
#endif
#ifndef B_SET
#define B_SET BIT_SET
#endif
#ifndef B_AND
#define B_AND BIT_AND
#endif
#ifndef B_OR
#define B_OR BIT_OR
#endif
#ifndef B_INVERT
#define B_INVERT BIT_NOT( BIT_DST)
#endif

#define EH	"eh?\n"
#define dprintf	if(debug)fprintf

/* bitmap chess 96x224x1*/

unsigned char chessbits[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 
	0x00, 0x80, 0x00, 0x00, 0x01, 0xa4, 0x92, 0x49, 0x25, 0x00, 0x00, 
	0x00, 0x00, 0x80, 0x00, 0x00, 0x01, 0xc9, 0x24, 0x92, 0x49, 0x3f, 
	0xff, 0xfe, 0x00, 0x80, 0x00, 0x00, 0x01, 0x92, 0x49, 0x24, 0x93, 
	0x40, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00, 0x01, 0xa4, 0x92, 0x49, 
	0x25, 0x40, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00, 0x01, 0xc9, 0x24, 
	0x92, 0x49, 0x4e, 0x1c, 0x39, 0x00, 0x80, 0x00, 0x00, 0x01, 0x92, 
	0x49, 0x24, 0x93, 0x51, 0x3e, 0x45, 0x00, 0x80, 0x00, 0x00, 0x01, 
	0xa4, 0x92, 0x49, 0x25, 0x51, 0x3e, 0x45, 0x00, 0x80, 0x00, 0x00, 
	0x01, 0xc9, 0x24, 0x92, 0x49, 0x51, 0x3e, 0x45, 0x00, 0x80, 0x00, 
	0x00, 0x01, 0x92, 0x49, 0x24, 0x93, 0x51, 0x3e, 0x45, 0x00, 0x80, 
	0x00, 0x00, 0x01, 0xa4, 0x92, 0x49, 0x25, 0x51, 0x3e, 0x45, 0x00, 
	0x80, 0x00, 0x00, 0x01, 0xc9, 0x24, 0x92, 0x49, 0x51, 0x3e, 0x45, 
	0x00, 0x80, 0x00, 0x00, 0x01, 0x92, 0x49, 0x24, 0x93, 0x51, 0x3e, 
	0x45, 0x00, 0x80, 0x00, 0x00, 0x01, 0xa4, 0x92, 0x49, 0x25, 0x51, 
	0x3e, 0x45, 0x00, 0x80, 0x00, 0x00, 0x01, 0xc9, 0x24, 0x92, 0x49, 
	0x51, 0x3e, 0x45, 0x00, 0x80, 0x00, 0x00, 0x01, 0x92, 0x49, 0x24, 
	0x93, 0x51, 0x3e, 0x45, 0x00, 0x80, 0x00, 0x00, 0x01, 0xa4, 0x92, 
	0x49, 0x25, 0x51, 0x3e, 0x45, 0x00, 0x80, 0x00, 0x00, 0x01, 0xc9, 
	0x24, 0x92, 0x49, 0x51, 0x3e, 0x45, 0x00, 0x80, 0x00, 0x00, 0x01, 
	0x92, 0x49, 0x24, 0x93, 0x51, 0x3e, 0x45, 0x00, 0x80, 0x00, 0x00, 
	0x01, 0xa4, 0x92, 0x49, 0x25, 0x4e, 0x1c, 0x39, 0x00, 0x80, 0x00, 
	0x00, 0x01, 0xc9, 0x24, 0x92, 0x49, 0x40, 0x00, 0x01, 0x00, 0x80, 
	0x00, 0x00, 0x01, 0x92, 0x49, 0x24, 0x93, 0x40, 0x00, 0x01, 0x00, 
	0x80, 0x00, 0x00, 0x01, 0xa4, 0x92, 0x49, 0x25, 0x40, 0x00, 0x01, 
	0x00, 0x80, 0x00, 0x00, 0x01, 0xc9, 0x24, 0x92, 0x49, 0x40, 0x00, 
	0x01, 0x00, 0x80, 0x00, 0x00, 0x01, 0x92, 0x49, 0x24, 0x93, 0x40, 
	0x00, 0x01, 0x00, 0x80, 0x00, 0x00, 0x01, 0xa4, 0x92, 0x49, 0x25, 
	0x40, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00, 0x01, 0xc9, 0x24, 0x92, 
	0x49, 0x40, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00, 0x01, 0x92, 0x49, 
	0x24, 0x93, 0x40, 0x0d, 0x59, 0x00, 0x80, 0x00, 0x00, 0x01, 0xa4, 
	0x92, 0x49, 0x25, 0x40, 0x09, 0x55, 0x00, 0x80, 0x00, 0x00, 0x01, 
	0xc9, 0x24, 0x92, 0x49, 0x40, 0x18, 0xd5, 0x00, 0x80, 0x00, 0x00, 
	0x01, 0x92, 0x49, 0x24, 0x93, 0x40, 0x00, 0x01, 0x00, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xfe, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 
	0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xff, 0xfc, 0x3f, 0xff, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x03, 0xc0, 
	0x00, 0xff, 0xf8, 0x1f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 
	0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x0b, 0xf0, 0x00, 0x00, 
	0x08, 0xb0, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x0b, 0xf0, 0x00, 
	0x00, 0x08, 0x50, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x0b, 0xf0, 
	0x00, 0x00, 0x08, 0xb0, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x05, 
	0xe0, 0x00, 0x00, 0x04, 0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 
	0x02, 0xc0, 0x00, 0x00, 0x03, 0xc0, 0x00, 0xff, 0xe0, 0x07, 0xff, 
	0x00, 0x0d, 0xf0, 0x00, 0x00, 0x0c, 0x70, 0x00, 0xff, 0x80, 0x01, 
	0xff, 0x00, 0x3b, 0xfc, 0x00, 0x00, 0x30, 0xac, 0x00, 0xff, 0x80, 
	0x01, 0xff, 0x00, 0x0d, 0xf0, 0x00, 0x00, 0x0c, 0x70, 0x00, 0xff, 
	0x80, 0x01, 0xff, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x03, 0xc0, 0x00, 
	0xff, 0xe0, 0x07, 0xff, 0x00, 0x05, 0x40, 0x00, 0x00, 0x04, 0xa0, 
	0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 
	0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 
	0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 
	0x00, 0x04, 0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 
	0x00, 0x00, 0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 
	0xe0, 0x00, 0x00, 0x04, 0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 
	0x05, 0xe0, 0x00, 0x00, 0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 
	0x00, 0x0f, 0xf0, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0xff, 0xe0, 0x07, 
	0xff, 0x00, 0x15, 0x58, 0x00, 0x00, 0x10, 0x58, 0x00, 0xff, 0xc0, 
	0x03, 0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0xff, 
	0x80, 0x01, 0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 0x2c, 0x00, 
	0xff, 0x80, 0x01, 0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 0x54, 
	0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 
	0xfc, 0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfe, 0x7f, 
	0xff, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0xff, 0xfc, 
	0x3f, 0xff, 0x00, 0x02, 0xc0, 0x00, 0x00, 0x02, 0x40, 0x00, 0xff, 
	0xf8, 0x1f, 0xff, 0x00, 0x02, 0xc0, 0x00, 0x00, 0x02, 0xc0, 0x00, 
	0xff, 0xf8, 0x1f, 0xff, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 
	0x00, 0xff, 0xfc, 0x3f, 0xff, 0x00, 0x02, 0xc0, 0x00, 0x00, 0x02, 
	0x40, 0x00, 0xff, 0xf8, 0x1f, 0xff, 0x00, 0x05, 0x20, 0x00, 0x00, 
	0x04, 0xe0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x0a, 0x70, 0x00, 
	0x00, 0x09, 0xd0, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x0c, 0xf0, 
	0x00, 0x00, 0x0b, 0xb0, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x07, 
	0xe0, 0x00, 0x00, 0x04, 0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 
	0x03, 0xc0, 0x00, 0x00, 0x03, 0xc0, 0x00, 0xff, 0xf8, 0x1f, 0xff, 
	0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 
	0xff, 0x00, 0x0b, 0xf0, 0x00, 0x00, 0x08, 0x50, 0x00, 0xff, 0xe0, 
	0x07, 0xff, 0x00, 0x05, 0x40, 0x00, 0x00, 0x07, 0xe0, 0x00, 0xff, 
	0xe0, 0x07, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0xa0, 0x00, 
	0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0x60, 
	0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 
	0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 
	0x04, 0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 
	0x00, 0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 
	0x00, 0x00, 0x04, 0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 
	0xe0, 0x00, 0x00, 0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 
	0x0f, 0xf0, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0xff, 0xe0, 0x07, 0xff, 
	0x00, 0x15, 0x58, 0x00, 0x00, 0x10, 0x58, 0x00, 0xff, 0xc0, 0x03, 
	0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0xff, 0x80, 
	0x01, 0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 0x2c, 0x00, 0xff, 
	0x80, 0x01, 0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 0x54, 0x00, 
	0xff, 0x80, 0x01, 0xff, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 
	0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 
	0x01, 0xff, 0x00, 0x3b, 0xdc, 0x00, 0x00, 0x3b, 0xdc, 0x00, 0xff, 
	0x80, 0x01, 0xff, 0x00, 0x2b, 0xdc, 0x00, 0x00, 0x2a, 0x54, 0x00, 
	0xff, 0x80, 0x01, 0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x2e, 0x7c, 
	0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 
	0x14, 0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 
	0x20, 0x2c, 0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x2a, 0xac, 0x00, 
	0x00, 0x3f, 0xfc, 0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x17, 0xf8, 
	0x00, 0x00, 0x10, 0x58, 0x00, 0xff, 0xc0, 0x03, 0xff, 0x00, 0x0b, 
	0xf0, 0x00, 0x00, 0x08, 0x30, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 
	0x05, 0x40, 0x00, 0x00, 0x07, 0xe0, 0x00, 0xff, 0xe0, 0x07, 0xff, 
	0x00, 0x0f, 0xf0, 0x00, 0x00, 0x08, 0x50, 0x00, 0xff, 0xe0, 0x07, 
	0xff, 0x00, 0x05, 0x40, 0x00, 0x00, 0x07, 0xe0, 0x00, 0xff, 0xe0, 
	0x07, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0xa0, 0x00, 0xff, 
	0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0x60, 0x00, 
	0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0xa0, 
	0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 
	0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 
	0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 
	0x00, 0x04, 0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 
	0x00, 0x00, 0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x0f, 
	0xf0, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 
	0x15, 0x58, 0x00, 0x00, 0x10, 0x58, 0x00, 0xff, 0xc0, 0x03, 0xff, 
	0x00, 0x2f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0xff, 0x80, 0x01, 
	0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 0x2c, 0x00, 0xff, 0x80, 
	0x01, 0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 0x54, 0x00, 0xff, 
	0x80, 0x01, 0xff, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 
	0xff, 0x80, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x8f, 0xff, 0xff, 0x00, 
	0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0xff, 0x87, 0xff, 0xff, 
	0x00, 0x38, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0xff, 0x83, 0xff, 
	0xff, 0x00, 0x68, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0xff, 0x00, 
	0xff, 0xff, 0x00, 0xaf, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0xfe, 
	0x00, 0x7f, 0xff, 0x00, 0xbf, 0x80, 0x00, 0x00, 0xa5, 0x80, 0x00, 
	0xfe, 0x00, 0x3f, 0xff, 0x01, 0xd8, 0xc0, 0x00, 0x01, 0x28, 0x40, 
	0x00, 0xfc, 0x00, 0x1f, 0xff, 0x01, 0xbe, 0xe0, 0x00, 0x01, 0x43, 
	0xa0, 0x00, 0xfc, 0x00, 0x0f, 0xff, 0x01, 0xbf, 0xf0, 0x00, 0x01, 
	0x40, 0x90, 0x00, 0xfc, 0x00, 0x07, 0xff, 0x01, 0xbf, 0xf8, 0x00, 
	0x01, 0x40, 0x08, 0x00, 0xfc, 0x00, 0x03, 0xff, 0x01, 0xbf, 0xfc, 
	0x00, 0x01, 0x40, 0x04, 0x00, 0xfc, 0x00, 0x01, 0xff, 0x01, 0xdf, 
	0xf6, 0x00, 0x01, 0x20, 0x0a, 0x00, 0xfc, 0x00, 0x00, 0xff, 0x00, 
	0xdd, 0xfe, 0x00, 0x00, 0xa1, 0x02, 0x00, 0xfe, 0x00, 0x00, 0xff, 
	0x00, 0xde, 0x3e, 0x00, 0x00, 0x90, 0xe2, 0x00, 0xfe, 0x00, 0x00, 
	0xff, 0x00, 0x6f, 0xdc, 0x00, 0x00, 0x50, 0xbc, 0x00, 0xff, 0x00, 
	0x01, 0xff, 0x00, 0x6b, 0xe0, 0x00, 0x00, 0x54, 0x60, 0x00, 0xff, 
	0x00, 0x03, 0xff, 0x00, 0x6f, 0xf0, 0x00, 0x00, 0x50, 0x30, 0x00, 
	0xff, 0x00, 0x07, 0xff, 0x00, 0x6d, 0xf0, 0x00, 0x00, 0x52, 0x50, 
	0x00, 0xff, 0x00, 0x07, 0xff, 0x00, 0x2f, 0xf8, 0x00, 0x00, 0x30, 
	0x28, 0x00, 0xff, 0x80, 0x03, 0xff, 0x00, 0x15, 0xf8, 0x00, 0x00, 
	0x12, 0x58, 0x00, 0xff, 0xc0, 0x03, 0xff, 0x00, 0x17, 0xf8, 0x00, 
	0x00, 0x10, 0x28, 0x00, 0xff, 0xc0, 0x03, 0xff, 0x00, 0x0a, 0xa0, 
	0x00, 0x00, 0x0f, 0xf0, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x1f, 
	0xf8, 0x00, 0x00, 0x10, 0x58, 0x00, 0xff, 0xc0, 0x03, 0xff, 0x00, 
	0x2a, 0xac, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0xff, 0x80, 0x01, 0xff, 
	0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 0x2c, 0x00, 0xff, 0x80, 0x01, 
	0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 0x54, 0x00, 0xff, 0x80, 
	0x01, 0xff, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0xff, 
	0x80, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xff, 0x80, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xf8, 0x1f, 0xff, 0x00, 0x03, 
	0xc0, 0x00, 0x00, 0x03, 0xc0, 0x00, 0xff, 0xf8, 0x1f, 0xff, 0x00, 
	0x02, 0xc0, 0x00, 0x00, 0x02, 0x40, 0x00, 0xff, 0xe0, 0x07, 0xff, 
	0x00, 0x0e, 0xf0, 0x00, 0x00, 0x0e, 0xb0, 0x00, 0xff, 0xe0, 0x07, 
	0xff, 0x00, 0x09, 0xf0, 0x00, 0x00, 0x08, 0x50, 0x00, 0xff, 0xe0, 
	0x07, 0xff, 0x00, 0x0e, 0xf0, 0x00, 0x00, 0x0e, 0xb0, 0x00, 0xff, 
	0xe0, 0x07, 0xff, 0x00, 0x02, 0xc0, 0x00, 0x00, 0x02, 0x40, 0x00, 
	0xff, 0xf0, 0x07, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0xa0, 
	0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x0b, 0xf0, 0x00, 0x00, 0x08, 
	0x50, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x0b, 0xf0, 0x00, 0x00, 
	0x08, 0xb0, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x05, 0xe0, 0x00, 
	0x00, 0x04, 0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x03, 0x40, 
	0x00, 0x00, 0x03, 0xc0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 
	0xe0, 0x00, 0x00, 0x04, 0x20, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 
	0x03, 0xc0, 0x00, 0x00, 0x03, 0xc0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 
	0x00, 0x05, 0x40, 0x00, 0x00, 0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 
	0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0x60, 0x00, 0xff, 0xf0, 
	0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0xa0, 0x00, 0xff, 
	0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0x60, 0x00, 
	0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0xa0, 
	0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 
	0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 
	0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x0b, 0xf0, 0x00, 
	0x00, 0x0f, 0xf0, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x15, 0x58, 
	0x00, 0x00, 0x10, 0x58, 0x00, 0xff, 0xc0, 0x03, 0xff, 0x00, 0x2f, 
	0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 
	0x2f, 0xfc, 0x00, 0x00, 0x20, 0x2c, 0x00, 0xff, 0x80, 0x01, 0xff, 
	0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 0x54, 0x00, 0xff, 0x80, 0x01, 
	0xff, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0xff, 0x80, 
	0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 
	0x80, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xff, 0xf8, 0x1f, 0xff, 0x00, 0x01, 0x80, 
	0x00, 0x00, 0x01, 0x80, 0x00, 0xff, 0xf8, 0x1f, 0xff, 0x00, 0x02, 
	0xc0, 0x00, 0x00, 0x02, 0x40, 0x00, 0xff, 0xf8, 0x1f, 0xff, 0x00, 
	0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0xff, 0x80, 0x01, 0xff, 
	0x00, 0x32, 0xcc, 0x00, 0x00, 0x32, 0x4c, 0x00, 0xff, 0x80, 0x01, 
	0xff, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x2c, 0x34, 0x00, 0xff, 0x80, 
	0x01, 0xff, 0x00, 0x25, 0x5c, 0x00, 0x00, 0x23, 0xec, 0x00, 0xff, 
	0x80, 0x01, 0xff, 0x00, 0x2f, 0xfc, 0x00, 0x00, 0x20, 0x54, 0x00, 
	0xff, 0x80, 0x01, 0xff, 0x00, 0x37, 0xfc, 0x00, 0x00, 0x20, 0x2c, 
	0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x1b, 0xf8, 0x00, 0x00, 0x10, 
	0x58, 0x00, 0xff, 0xc0, 0x03, 0xff, 0x00, 0x0f, 0xf0, 0x00, 0x00, 
	0x0c, 0x30, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x02, 0xc0, 0x00, 
	0x00, 0x03, 0xc0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 
	0x00, 0x00, 0x04, 0x60, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x02, 
	0x80, 0x00, 0x00, 0x03, 0xc0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 
	0x07, 0xe0, 0x00, 0x00, 0x04, 0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 
	0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0x60, 0x00, 0xff, 0xf0, 0x0f, 
	0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0xa0, 0x00, 0xff, 0xf0, 
	0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0x60, 0x00, 0xff, 
	0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0xa0, 0x00, 
	0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 0x60, 
	0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x05, 0xe0, 0x00, 0x00, 0x04, 
	0xa0, 0x00, 0xff, 0xf0, 0x0f, 0xff, 0x00, 0x0d, 0xf0, 0x00, 0x00, 
	0x0f, 0xf0, 0x00, 0xff, 0xe0, 0x07, 0xff, 0x00, 0x1b, 0xf8, 0x00, 
	0x00, 0x10, 0x58, 0x00, 0xff, 0xc0, 0x03, 0xff, 0x00, 0x35, 0x5c, 
	0x00, 0x00, 0x3f, 0xfc, 0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 0x2f, 
	0xfc, 0x00, 0x00, 0x20, 0x2c, 0x00, 0xff, 0x80, 0x01, 0xff, 0x00, 
	0x2f, 0xfc, 0x00, 0x00, 0x20, 0x54, 0x00, 0xff, 0x80, 0x01, 0xff, 
	0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0xff, 0x80, 0x01, 
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 
	0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 
	0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xff, 0xff, 0xff, 0xff
};

int ifd, ofd;
FILE *ifp, *ofp;
char buf[512];
char newboard[BOARDSIZE], board[BOARDSIZE];
int w_x, w_y, w_w, w_h;
unsigned click1;
int clkx, clky;
int first;
char chess_name[] = "/usr/games/chess";
char pieces[] = "pbrnkq";
char iobuf[64], iobuf1[64];

int debug;

main(argc, argv)
int argc;
char **argv;
{
	FILE *fdopen();

	debug = getenv("DEBUG");
	if (argc>1 && strcmp(argv[1],"-f")==0)
		first = 1;
	else
		first = 0;
	mgr_startup();
	fork_chess(&ifd, &ofd);
	startup();
	reshape_event(); /* force a reshape */
	setup_int();
	while(1) do_events();
}

reshape_event()
{

	dprintf(stderr,"Reshape\n");
	get_size(&w_x, &w_y, &w_w, &w_h);
	m_shapewindow(w_x, w_y, WIN_W, WIN_H);
	m_clear();
	m_func(B_COPY);
	draw_blank();
	draw_board(board);
	draw_moves();
	click1 = 0;
}

draw_board(b)
char *b;
{
	register unsigned x, y;

	for (y=0; y < 8; y++) 
		for (x=0; x < 8; x++) 
			draw_piece(x,y,b[8*y+x]);
}

draw_moves()
{
	m_func(B_SET);
	m_go(MOVESX, MOVESY);
	m_draw(MOVESX + MOVESW, MOVESY);
	m_draw(MOVESX + MOVESW, MOVESY + MOVESH);
	m_draw(MOVESX, MOVESY + MOVESH);
	m_draw(MOVESX, MOVESY);
	m_textregion(MOVESX+2, MOVESY+2, MOVESX+MOVESW-2, MOVESY+MOVESH-14);
	m_flush();
}


draw_piece(x, y, b)
register unsigned x, y;
register char b;
{
	register unsigned index, piecex, piecey;

	m_func(B_COPY);
	if ((x+y)&1) {
		m_bitcopyto(0, 0, PIECEW, PIECEH,
		PIECEW, 0, SCRATCH, MAPNUM);
	}
	else {
		m_bitcopyto(0, 0, PIECEW, PIECEH,
			0, 0, SCRATCH, MAPNUM);
	}
	index = 8*y+x;
	if (isalpha(b)) {
		if (islower(b)) piecex = PIECEW;
		else piecex = 0;
		piecey = find_char(pieces,
			(isupper(b) ? tolower(b) : b))+1;
		piecey *= PIECEH;
		m_func(B_AND);
		m_bitcopyto(0, 0, PIECEW, PIECEH, PIECEW*2,
			piecey, SCRATCH, MAPNUM);
		m_func(B_OR);
		m_bitcopyto(0, 0, PIECEW, PIECEH, piecex,
					piecey, SCRATCH, MAPNUM);
		m_func(B_COPY);
	}
	m_bitcopyto(x*PIECEW, y*PIECEH, PIECEW, PIECEH, 0, 0, 0, SCRATCH);
}

find_char(s, c)
char *s, c;
{
	register int i;

	for (i=0; *s && *s != c; i++) s++;
	if (*s) return(i);
	return(-1);
}

draw_blank()
{
	register unsigned x, y;

	for (y=0; y < 2; y++) {
		for (x=0; x < 8; x++) {
			/* fastest way to do checkerboard */
			if ((x+y)&1) {
				m_bitcopyto(x*PIECEW,y*PIECEH,PIECEW,PIECEH,
					PIECEW, 0, 0, MAPNUM);
			}
			else {
				m_bitcopyto(x*PIECEW,y*PIECEH,PIECEW,PIECEH,
					0, 0, 0, MAPNUM);
			}
		}
	}
	m_bitcopy(0, 2*PIECEH, 8*PIECEW, 2*PIECEH, 0, 0);
	m_bitcopy(0, 4*PIECEH, 8*PIECEW, 4*PIECEH, 0, 0);
	m_flush();
}

mgr_startup()
{
	/* do the mgr-related initialization */
	int cleanup();

	m_setup(M_MODEOK);
	m_push(P_BITMAP|P_EVENT|P_FLAGS|P_POSITION|P_CURSOR);
	m_setcursor(CS_INVIS);
	m_setmode(M_ABS);

	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGCHLD, cleanup);

	m_setevent(RESHAPE, "R\n");
	m_setevent(REDRAW, "r\n");
	m_ttyset();

	load_map();
	dprintf(stderr,"startup\n");
}

cleanup()
{
	dprintf(stderr,"cleanup\n");
	m_ttyreset();
	m_textreset();
	m_pop();
	m_flush();
	exit(0);
}

load_map()
{
	int local_mode;

	ioctl(fileno(m_termout), TIOCLGET, &local_mode);
	local_mode |= LLITOUT;
	ioctl(fileno(m_termout), TIOCLSET, &local_mode);

	m_bitldto(MAPW, MAPH, 0, 0, MAPNUM, sizeof(chessbits));
	m_flush();
	write(fileno(m_termout), chessbits, sizeof(chessbits));
	m_flush();

	local_mode &= ~LLITOUT;
	ioctl(fileno(m_termout), TIOCLSET, &local_mode);
}

startup()
{
	int c;
	char buf[64];

	ifp = fdopen(ifd, "r"); /* convert file decsriptor to a stream */
	ofp = fdopen(ofd, "w");

	setlinebuf(ifp); /* make the buffering line oriented */
	setlinebuf(ofp);

	fputs("algebraic\n", ofp);
	fgets(buf, sizeof(buf), ifp);
	dprintf(stderr,"<algebraic> got: %s\n",buf);
	/* this reads the title "Chess\n" */
	if (first) {
		fputs("first\n", ofp);
		fgets(iobuf1, sizeof(iobuf1), ifp);
		dprintf(stderr,"<first> got: %s\n",iobuf1);
	}
	getboard(board);
}

do_diffs()
{
	register unsigned  x, y;

	getboard(newboard);
	for (y = 0; y < 8; y++)
		for(x = 0; x < 8; x++)
			if (board[8*y+x] != newboard[8*y+x])
				draw_piece(x, y, newboard[8*y+x]);
	m_flush();
	copyboard(board, newboard);
}

do_move(x1,y1,x2,y2)
int x1,y1,x2,y2;
{
	register int i;

	fprintf(ofp, "%c%d%c%d\n", 'a'+ x1, 8 - y1, 'a' + x2, 8 - y2);
	dprintf(stderr, "sent: %c%d%c%d\n", 'a'+ x1, 8 - y1, 'a' + x2, 8 - y2);
	if (first) {
		fgets(iobuf, sizeof(iobuf), ifp);
		dprintf(stderr,"got: %s\n",iobuf);
		if ((strncmp(iobuf, "Illegal move", 12)) == 0)
			return(1);
		for (i=0; iobuf1[i] != '\n'; i++);
		iobuf1[i] = ' ';
		for (i=0; !isalpha(iobuf[i]); i++);
		strcat(iobuf1, iobuf + i);
		fputs(iobuf1, m_termout);
		dprintf(stderr,"sent: %s\n",iobuf1);
		fgets(iobuf1, sizeof(iobuf1), ifp);
		dprintf(stderr,"got: %s\n",iobuf1);
	}
	else {
		fgets(iobuf, sizeof(iobuf), ifp);
		dprintf(stderr,"got: %s\n",iobuf);
		if ((strncmp(iobuf, "Illegal move", 12)) == 0)
			return(1);
		fgets(iobuf1, sizeof(iobuf1), ifp);
		dprintf(stderr,"got: %s\n",iobuf1);
		for (i=0; iobuf[i] != '\n'; i++);
		iobuf[i] = ' ';
		for (i=0; !isalpha(iobuf1[i]); i++);
		strcat(iobuf, iobuf1 + i);
		fputs(iobuf, m_termout);
		dprintf(stderr,"sent: %s\n",iobuf);
	}
	return(0);
}

getboard(b)
char *b;
{
	int i, j;
	char buf[64];

	/* a carriage return is a request for a board.
	 * the board has the form:
	 * n  c c c c c c c c
	 * m  c c c c c c c c
	 * .
	 * .
	 * .
	 * Followed by 3 more lines of stuff.
	 * So, I read a line of board, and extract the
	 * pieces (or squares) which are in odd columns
	 * from 3 on.
	 */
	fputc('\n', ofp);
	for (i=0; i < 8; i++) {
		fgets(buf, sizeof(buf), ifp);
		for (j=0; j < 8; j++) {
			b[8*i+j] = buf[2*j + 3];
		}
	}
	fputs("Z\n", ofp);
	/* consume until eh? */
	while (fgets(buf, sizeof(buf), ifp) && strcmp(buf,EH)!=0);
	dprintf(stderr,"got board\n");
}

copyboard(d, s)
register char *d, *s;
{
	/* copy board s into board d */
	register int i;

	i = BOARDSIZE;
	do {
		*d++ = *s++;
	} while(--i > 0);
}

fatal(s)
char *s;
{
	fprintf(stderr, s);
	exit(1);
}



fork_chess(infd, outfd)
int *infd, *outfd;
{
	int fdout[2], fdin[2];
	if (pipe(fdout) == -1 || pipe(fdin) == -1) {
		fatal("Pipe failed.\n");
	}
	switch(fork()) {
	case -1:
		fatal("Fork failed.\n");
		break; /* statement never reached */
	case 0:
		close(0);
		if (dup(fdout[0]) != 0) {
			fatal("Couldn't dup stdout\n");
		}
		close(1);
		if (dup(fdin[1]) != 1) {
			fatal("Couldn't dup stdin\n");
		}
		close(fdout[0]);
		close(fdout[1]);
		close(fdin[0]);
		close(fdin[1]);
		execlp(chess_name, chess_name, NULL);
		fatal("Couldn't start chess.\n");
		break;
	} /* end switch */
	close(fdout[0]); close(fdin[1]);
	*outfd = fdout[1];
	*infd = fdin[0];
}

/* button interface */

setup_int()
{
	m_setevent(BUTTON_2, "m%p\n");
	m_setevent(BUTTON_2U, "M%p\n");
}

do_events()
{
	char line[64];
	int mx, my;

	if (m_gets(line) != NULL) {
		switch(*line) {
		case 'Q':
		case 'q':
			cleanup();
			break;
		case 'R':
			reshape_event();
			break;
		case 'm':
			if (click1) {
				sscanf(line+1, "%d %d", &mx, &my);
				mx /= PIECEW;
				my /= PIECEH;
				if (mx >= 0 && mx < 8 && my >= 0 && my < 8) {
					click1 = 0;
					m_func(B_INVERT);
					m_bitwrite(mx*PIECEW, my*PIECEH,
						PIECEW,PIECEH);
					m_flush();
					if (!(mx == clkx && my == clky)) {
						fprintf(m_termout,"(working)\r"); fflush(m_termout);
						if (do_move(clkx, clky, mx, my)) {
							fprintf(m_termout,"huh?     \r"); fflush(m_termout);
							}
						m_bitwrite(mx*PIECEW, my*PIECEH,
							PIECEW,PIECEH);
						m_bitwrite(clkx*PIECEW, clky*PIECEH,
							PIECEW,PIECEH);
						do_diffs();
					}
				}
			}
			else {
				sscanf(line+1, "%d %d", &mx, &my);
				mx /= PIECEW;
				my /= PIECEH;
				if (mx >= 0 && mx < 8 && my >= 0 && my < 8) {
					click1 = 1;
					clkx = mx;
					clky = my;
					m_func(B_INVERT);
					m_bitwrite(mx*PIECEW, my*PIECEH,
						PIECEW,PIECEH);
				}
			}
		}
	}
}
