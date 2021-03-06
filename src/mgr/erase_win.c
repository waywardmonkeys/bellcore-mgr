/*{{{}}}*/
/*{{{  Notes*/
/* erase a pixrect to background pattern */
/*}}}  */
/*{{{  #includes*/
#include <stdio.h>
#include <mgr/bitblit.h>

#include "defs.h"

#include "icon_server.h"
#include "mgr.h"
/*}}}  */

/*{{{  Bit_pattern -- fill DST bitmap with SRC, preserving alignment*/
static void
Bit_pattern(BITMAP *dst, int dx, int dy, int wide, int high, int func, BITMAP *src)
{
  int incr;
  int sw = BIT_WIDE(src);
  int sh = BIT_HIGH(src);
  int x = BIT_X(dst) + dx;
  int y = BIT_Y(dst) + dy;
  int xdel = x % sw;
  int ydel = y % sh;
  int de;

  dx -= xdel, wide += xdel;
  de = dx + wide;

  /* get partial strip */

  if (ydel) {
    for (incr = dx; incr < de; incr += sw)
      bit_blit(dst, incr, dy - ydel, sw, sh, func, src, 0, 0);
    dy += sh - ydel;
  }

  /* get 1st strip */

  for (incr = dx; incr < de; incr += sw)
    bit_blit(dst, incr, dy, sw, sh, func, src, 0, 0);

  /* get the rest */

  de = dy + high;
  for (incr = dy + sh; incr < de; incr += sh, sh <<= 1)
    bit_blit(dst, dx, incr, wide, sh, func, dst, dx, dy);
}
/*}}}  */

/*{{{  erase_win*/
void erase_win(BITMAP *map)
{
  Bit_pattern(
      map,
      0, 0,
      BIT_WIDE(map), BIT_HIGH(map),
      BUILDOP(BIT_SRC, color_map[ROOT_COLOR_FG], color_map[ROOT_COLOR_BG]),
      pattern);
}
/*}}}  */
