/*{{{}}}*/
/*{{{  Notes*/
/*                        Copyright (c) 1987 Bellcore
 *                            All Rights Reserved
 *       Permission is granted to copy or use this program, EXCEPT that it
 *       may not be sold for profit, the copyright notice must be reproduced
 *       on copies, and credit should be given to Bellcore where it is due.
 *       BELLCORE MAKES NO WARRANTY AND ACCEPTS NO LIABILITY FOR THIS PROGRAM.
*/

/* cut and paste text */
/*}}}  */
/*{{{  #includes*/
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <mgr/bitblit.h>
#include <mgr/font.h>

#include "defs.h"
#include "event.h"

#include "Write.h"
#include "do_button.h"
#include "do_event.h"
#include "font_subs.h"
#include "get_text.h"
#include "icon_server.h"
#include "intersect.h"
#include "mouse_get.h"
#include "subs.h"
#include "proto.h"
/*}}}  */
/*{{{  #defines*/
#define isw(c) (c == ' ' || c == '\r' || c == '\n' || c == '\t')
#define MAXROWS 64 /* greatest char height */
#define MAXCOLS 32 /* widest char (bits in u-long) */
/*}}}  */

/*{{{  variables*/
static BITMAP *glyph;                /* spot for glyph comparison */
static unsigned long data[MAXROWS];  /* bit data for glyph */
static BITMAP *check;                /* other spot for glyph comparison */
static unsigned long data2[MAXROWS]; /* bit data for other glyph */

static struct entry **table; /* hash table */
/*}}}  */

/*{{{  get_hash -- given bitmap, get hash code*/
int get_hash(
    BITMAP *map,
    int x,
    int y,
    int w,
    int h,  /* where in map */
    int how /* 0-> normal, 1->inverted */
    )
{
  unsigned long sum = 0;
  int j;
  bit_blit(glyph, 0, 0, 32, h, BIT_CLR, NULL_DATA, 0, 0);
  bit_blit(glyph, 32 - w, 0, w, h, how ? BIT_NOT(BIT_SRC) : BIT_SRC, map, x, y);
  for (j = 0; j < h; j++)
    sum += data[j] * (j + 1);
  return (sum % H_SIZE);
}
/*}}}  */

/*{{{  enter -- enter a glyph into the hash table*/
static void
enter(int item, int value, int type)
{
  struct entry *entry;

  if (table[item] == NULL) {
    table[item] = malloc(sizeof(struct entry));
    table[item]->value = value;
    table[item]->type = type;
    table[item]->next = NULL;
  } else {
    for (entry = table[item]; entry->next; entry = entry->next)
      ;
    entry->next = malloc(sizeof(struct entry));
    entry->next->value = value;
    entry->next->type = type;
    entry->next->next = NULL;
  }
}
/*}}}  */
/*{{{  get_match -- find a character match in current font*/
static char
get_match(
    WINDOW *win,
    BITMAP *map, /* bitmap containing text */
    int x,
    int y,
    int w,
    int h /* position of glyph in "map" */
    )
{
  struct entry *entry;
  int code; /* hash code */
  int size = sizeof(data[0]) * h;

  code = get_hash(map, x, y, w, h, 0); /* leaves char in glyph */
  for (entry = table[code]; entry; entry = entry->next) {
    bit_blit(check, 32 - w, 0, w, h, BIT_SRC, W(font)->glyph[entry->value + entry->type * MAXGLYPHS], 0, 0);
    if (memcmp(data, data2, size) == 0) {
      return (entry->value);
    }
  }

  /* try inverse video version */

  code = get_hash(map, x, y, w, h, 1); /* leaves char in glyph */
  for (entry = table[code]; entry; entry = entry->next) {
    bit_blit(check, 32 - w, 0, w, h, BIT_SRC, W(font)->glyph[entry->value], 0, 0);
    if (memcmp(data, data2, size) == 0) {
      return (entry->value);
    }
  }
  return ('\0');
}
/*}}}  */
/*{{{  fixline -- change trailing white space into \n*/
static char *
fixline(char *s, char *pnt)
{
  while (*--pnt == ' ' && pnt > s)
    ;
  *++pnt = '\n';
  return (++pnt);
}
/*}}}  */
/*{{{  to_tabs -- change spaces to tabs*/
static char *
to_tabs(
    int pos,  /* starting col # */
    char *in, /* input str */
    char *out /* output str - tabs */
    )
{
  char *s = out;  /* start of out str */
  char c;         /* current input char */
  int spaces = 0; /* # pending spaces */

  dbgprintf('C', (stderr, "-> TABS"));
  while (pos++, c = *in++) {

    if (c == '\n' || c == '\r' || c == '\f') /* reset column counter */
      pos = 0;

    if (c == ' ') {
      spaces++;
      if (pos && pos % 8 == 0) { /* ' ' -> \t */
        c = '\t';
        dbgprintf('C', (stderr, "."));
        spaces = 0;
      }
    } else
      for (; spaces > 0; spaces--) { /* output spaces */
        *out++ = ' ';
      }

    if (spaces == 0)
      *out++ = c;
  }
  *out = '\0';
  dbgprintf('C', (stderr, "\n"));
  return (s);
}
/*}}}  */
/*{{{  oops -- can't cut, ring bell*/
static void
oops(WINDOW *win)
{
  CLEAR(W(window), BIT_NOT(BIT_DST));
  write(2, "\007", 1);
  CLEAR(W(window), BIT_NOT(BIT_DST));
}
/*}}}  */

/*{{{  paste -- stuff global buffer into input stream*/
void paste(void)
{
  if (snarf) {
    do_event(EVENT_PASTE, active, E_MAIN);
    Write(active->to_fd, snarf, strlen(snarf));
    dbgprintf('y', (stderr, "%s: Pasting [%s]\n", active->tty, snarf ? snarf : "EMPTY"));
  } else {
    dbgprintf('y', (stderr, "%s: Nothing to paste\n", active->tty));
  }
}
/*}}}  */
/*{{{  cut -- cut text from a window, put in global buffer*/
/*	mode == 0 -> normal rubber band cut */
/*	mode == 1 -> copy word, called from special mouse button */
/*	mode == 2 -> copy word, called from do_buckey */
int cut(int mode)
{
  int i, j;
  WINDOW *win = active;    /* window to cut text from */
  int count = 0;           /* # of snarfed chars */
  int errors = 0;          /* number of misses */
  int cols = 1, rows = 0;  /* rows and cols swept */
  int col, row;            /* starting col and row */
  int maxcol;              /* # of cols in row */
  int x, y;                /* bit position in bitmap */
  int hcode;               /* hash code */
  int button = BUTTON_SYS; /* button from move_mouse */
  int linelen;
  char c = 0;               /* matched char */
  char *pntr;               /* current char in line */
  char *line;               /* buffer to receive text */
  BITMAP *src, *shrunk_src; /* source bitmaps */

  if (!mode) {
    SETMOUSEICON(&mouse_cut);
    button = move_mouse(screen, mouse, &mousex, &mousey, 0);
    SETMOUSEICON(DEFAULT_MOUSE_CURSOR);
  }

  /* return immediately if window is not snarffable */

  for (win = active; win != NULL; win = W(next))
    if (mousein(mousex, mousey, win, 1))
      break;
  if (!win || ((W(flags) & W_SNARFABLE) == 0))
    return (0);

  /* initialize comparison registers */

  glyph = bit_alloc(32, FSIZE(high), data, 1);
  check = bit_alloc(32, FSIZE(high), data2, 1);

  bit_blit(check, 0, 0, 32, FSIZE(high), BIT_CLR, NULL_DATA, 0, 0);

  /* build hash table */

  if ((table = W(font)->table) == NULL) {
    dbgprintf('C', (stderr, "building cut table\n"));
    table = W(font)->table = malloc(sizeof(struct entry) * H_SIZE);
    (void)memset(table, 0, sizeof(struct entry) * H_SIZE);

    count = W(font)->head.type & 0x80 ? 4 : 1;
    for (j = 0; j < count; j++)
      for (i = FSIZE(start); i < FSIZE(start) + FSIZE(count); i++) {
        if (W(font)->glyph[i + MAXGLYPHS * j] && i >= ' ') {
          hcode = get_hash(W(font)->glyph[i + MAXGLYPHS * j], 0, 0, FSIZE(wide), FSIZE(high), 0);
          enter(hcode, i, j);
        }
      }
  }

  /* find cut region */

  if (!mode) {
    i = get_text(screen, mouse, mousex, mousey, &cols, &rows, win, E_SWTEXTT);
    if (i == 0) {
      do_button(0);
      return (0);
    }
  }
#if 0  /* not supported yet */
  else if (mode == 1)
  /* copy word, called from mouse click.  This could be bound to shift
   * left mouse, etc
   */
  {
    int b, x, y;

    do
      b = mouse_get(mouse, &x, &y);
    while (b != 0);
  }
#endif /* 0 */

  /* find extent of cut region */

  col = (mousex - (W(x0) + W(borderwid) + W(text.x))) / FSIZE(wide);
  maxcol = (W(text.wide) ? W(text.wide) : BIT_WIDE(W(window))) / FSIZE(wide);
  row = (mousey - (W(y0) + W(borderwid) + W(text.y))) / FSIZE(high);

  if (W(flags) & W_SNARFLINES && !mode) { /* snarf lines only */
    dbgprintf('C', (stderr, "Cutting lines only\n"));
    col = 0;
    cols = maxcol;
  }

  dbgprintf('C', (stderr, "Cut got %d,%d  %d x %d\n", col, row, cols, rows));

  /* prepare src bitmap */
  src = W(window);
  if (BIT_DEPTH(src) > 1) { /* color display */
    shrunk_src = bit_shrink(src, GETBCOLOR(W(style)));
    src = bit_create(shrunk_src,
        BIT_X(src), BIT_Y(src), BIT_WIDE(src), BIT_HIGH(src));
  } else
    shrunk_src = 0;

  /* look up characters */

  pntr = line = malloc(linelen = (1 + (1 + maxcol) * (rows + 1))); /* max possible cut */
  if (mode) {
    char *opntr, *startpntr;

    pntr += linelen / 2;
    opntr = pntr;
    y = W(text.y) + row * FSIZE(high);
    /* Search backwards */
    for (i = 0;; i--) {
      x = W(text.x) + (col + i) * FSIZE(wide);
      c = get_match(win, src, x, y, FSIZE(wide), FSIZE(high));
      if (c == 0) {
        oops(win);
        break;
      }
      if (isw(c))
        break;
      *(--pntr) = c;
      if (pntr <= line)
        break;
    }
    /* Search forwards */
    if (c) /* Not error? */
    {
      startpntr = pntr;
      pntr = opntr;
      for (i = 1; pntr < (line + linelen - 1); i++) {
        x = W(text.x) + (col + i) * FSIZE(wide);
        c = get_match(win, src, x, y, FSIZE(wide), FSIZE(high));
        if (c == 0) {
          oops(win);
          break;
        }
        if (isw(c))
          break;
        *pntr++ = c;
      }
      if (c) {
        *pntr = 0;
        /* Paste it in */
        write(active->to_fd, startpntr, pntr - startpntr);
        write(active->to_fd, " ", 1);
        free(line);
        /*      snarf = NULL; */
      }
    }
  } else
    switch (rows) {
    case 0: /* 1 row */
      y = W(text.y) + row * FSIZE(high);
      for (x = W(text.x) + col * FSIZE(wide), i = 0; i < cols; i++, x += FSIZE(wide)) {
        c = get_match(win, src, x, y, FSIZE(wide), FSIZE(high));
        *pntr++ = c ? c : (errors++, C_NOCHAR);
      }
      if (col + cols == maxcol && c == ' ')
        pntr = fixline(line, pntr);
      break;
    case 1: /* 2 rows */
      y = W(text.y) + row * FSIZE(high);
      for (x = W(text.x) + col * FSIZE(wide), i = 0; i < maxcol; i++, x += FSIZE(wide)) {
        c = get_match(win, src, x, y, FSIZE(wide), FSIZE(high));
        *pntr++ = c ? c : (errors++, C_NOCHAR);
      }
      pntr = fixline(line, pntr);

      y += FSIZE(high);
      for (x = W(text.x), i = 0; i < col + cols; i++, x += FSIZE(wide)) {
        c = get_match(win, src, x, y, FSIZE(wide), FSIZE(high));
        *pntr++ = c ? c : (errors++, C_NOCHAR);
      }
      if (col + cols == maxcol && c == ' ')
        pntr = fixline(line, pntr);
      break;

    default: /* > 2 rows */
      y = W(text.y) + row * FSIZE(high);
      for (x = W(text.x) + col * FSIZE(wide), i = 0; i < maxcol; i++, x += FSIZE(wide)) {
        c = get_match(win, src, x, y, FSIZE(wide), FSIZE(high));
        *pntr++ = c ? c : (errors++, C_NOCHAR);
      }
      pntr = fixline(line, pntr);

      for (j = 0; j < rows - 1; j++) {
        y += FSIZE(high);
        for (x = W(text.x), i = 0; i < maxcol; i++, x += FSIZE(wide)) {
          c = get_match(win, src, x, y, FSIZE(wide), FSIZE(high));
          *pntr++ = c ? c : (errors++, C_NOCHAR);
        }
        pntr = fixline(line, pntr);
      }

      y += FSIZE(high);
      for (x = W(text.x), i = 0; i < col + cols; i++, x += FSIZE(wide)) {
        c = get_match(win, src, x, y, FSIZE(wide), FSIZE(high));
        *pntr++ = c ? c : (errors++, C_NOCHAR);
      }
      if (col + cols == maxcol && c == ' ')
        pntr = fixline(line, pntr);

      break;
    }

  /* dont use bit_free */
  free(check);
  free(glyph);

  if (!mode) {
    *pntr = '\0';

    /* put text into snarf buffer */
    count = pntr - line;
    dbgprintf('C', (stderr, "snarfed %d chars, %d errors\n", count, errors));
    dbgprintf('C', (stderr, "snarfed [%s]\n", line));

    if ((!(W(flags) & W_SNARFHARD) && errors > 0) || 2 * errors > count) {
      oops(win);
      count = 0;
    } else {
      if (W(flags) & W_SNARFTABS)
        to_tabs(col, line, line);

      if (snarf && button < BUTTON_SYS) { /* add to cut buffer */
        char *tmp = malloc(strlen(snarf) + strlen(line) + 1);
        count += strlen(snarf);
        strcpy(tmp, snarf);
        strcat(tmp, line);
        free(snarf);
        free(line);
        snarf = tmp;
      } else if (snarf) { /* replace cut buffer */
        free(snarf);
        snarf = line;
      } else /* new cut buffer */
        snarf = line;

      /* send snarf events (if any) */
      id_message = W(pid);
      for (win = active; win != NULL; win = W(next))
        do_event(EVENT_SNARFED, win, E_MAIN);
    }
  }
  do_button(0);

  if (shrunk_src)
    bit_destroy(shrunk_src);
  return (count);
}

void rubber_band_cut(void)
{
  (void)cut(0);
}
/*}}}  */
/*{{{  zap_fhash -- zap a font hash table*/
void zap_fhash(struct font *fnt)
{
  struct entry *entry, *next;
  int i;

  if (fnt->table) {
    for (i = 0; i < H_SIZE; i++)
      for (entry = table[i]; entry; entry = next) {
        next = entry->next;
        free(entry);
      }
    free(fnt->table);
  }
}
/*}}}  */
