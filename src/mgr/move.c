/*{{{}}}*/
/*{{{  Notes*/
/*                        Copyright (c) 1987 Bellcore
 *                            All Rights Reserved
 *       Permission is granted to copy or use this program, EXCEPT that it
 *       may not be sold for profit, the copyright notice must be reproduced
 *       on copies, and credit should be given to Bellcore where it is due.
 *       BELLCORE MAKES NO WARRANTY AND ACCEPTS NO LIABILITY FOR THIS PROGRAM.
 */

/* move a window */
/*}}}  */
/*{{{  #includes*/
#include <stdio.h>
#include <mgr/bitblit.h>

#include "defs.h"
#include "event.h"

#include "border.h"
#include "do_event.h"
#include "mouse_get.h"
#include "move_box.h"
#include "shape.h"
/*}}}  */

/*{{{  move_window*/
void move_window(void)
{
  int button;
  int dx = BIT_WIDE(active->border);
  int dy = BIT_HIGH(active->border);
  int sx = active->x0;
  int sy = active->y0;

  move_box(screen, mouse, &sx, &sy, dx, dy, 0);

  /* adjust window state */

  mousex += sx - active->x0;
  mousey += sy - active->y0;

  shape(sx, sy, dx, dy);
#ifdef MGR_ALIGN
  if (dx != BIT_WIDE(active->border) || dy != BIT_HIGH(active->border))
    do_event(EVENT_SHAPE, active, E_MAIN);
  else
#endif
    do_event(EVENT_MOVE, active, E_MAIN);

  /* wait till button is released */

  do {
    button = mouse_get(mouse, &sx, &sy);
  } while (button != 0);
}
/*}}}  */
