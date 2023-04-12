/*
 * This file is part of the 'Simple Sokoban' project.
 *
 * MIT LICENSE
 *
 * Copyright (C) 2014-2023 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>             /* malloc() */
#include <string.h>             /* memcpy() */
#include <time.h>
#include <SDL2/SDL.h>           /* SDL       */

#include "gra.h"
#include "sok_core.h"
#include "save.h"
#include "data.h"           /* embedded assets (font, levels...) */
#include "gz.h"
#include "net.h"
#include "skin.h"

#define PVER "1.0.3"
#define PDATE "2014-2023"

#define INET_HOST "simplesok.osdn.io"
#define INET_PORT 80
#define INET_PATH "/netlevels/"

#define DEFAULT_SKIN "antique3"

#define debugmode 0

#define MAXLEVELS 4096
#define SCREEN_DEFAULT_WIDTH 800
#define SCREEN_DEFAULT_HEIGHT 600

#define DISPLAYCENTERED 1
#define NOREFRESH 2

#define DRAWSCREEN_REFRESH 1
#define DRAWSCREEN_PLAYBACK 2
#define DRAWSCREEN_PUSH 4
#define DRAWSCREEN_NOBG 8
#define DRAWSCREEN_NOTXT 16

#define DRAWSTRING_CENTER -1
#define DRAWSTRING_RIGHT -2
#define DRAWSTRING_BOTTOM -3

#define DRAWPLAYFIELDTILE_DRAWATOM 1
#define DRAWPLAYFIELDTILE_PUSH 2

#define BLIT_LEVELMAP_BACKGROUND 1

#define FONT_SPACE_WIDTH 12
#define FONT_KERNING -3

#define SELECTLEVEL_BACK -1
#define SELECTLEVEL_QUIT -2
#define SELECTLEVEL_LOADFILE -3
#define SELECTLEVEL_OK -4

enum normalizedkeys {
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_CTRL_UP,
  KEY_CTRL_DOWN,
  KEY_ENTER,
  KEY_BACKSPACE,
  KEY_PAGEUP,
  KEY_PAGEDOWN,
  KEY_HOME,
  KEY_END,
  KEY_ESCAPE,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_FULLSCREEN,
  KEY_F12,
  KEY_S,
  KEY_R,
  KEY_CTRL_C,
  KEY_CTRL_V,
  KEY_UNKNOWN
};

enum leveltype {
  LEVEL_INTERNAL,
  LEVEL_INTERNET,
  LEVEL_FILE
};


struct videosettings {
  unsigned short tilesize;
  int framedelay;
  int framefreq;
  const char *customskinfile;
};

/* returns the absolute value of the 'i' integer. */
static int absval(int i) {
  if (i < 0) return(-i);
  return(i);
}

/* uncompress a sokoban XSB string and returns a new malloced string with the decompressed version */
static char *unRLE(char *xsb) {
  char *res = NULL;
  size_t resalloc = 16;
  unsigned int reslen = 0;
  long x;
  int rlecnt = 1;
  res = malloc(resalloc);
  for (x = 0; xsb[x] != 0; x++) {
    if ((xsb[x] >= '0') && (xsb[x] <= '9')) {
      rlecnt = xsb[x] - '0';
    } else {
      for (; rlecnt > 0; rlecnt--) {
        if (reslen + 4 > resalloc) {
          resalloc *= 2;
          res = realloc(res, resalloc);
        }
        res[reslen++] = xsb[x];
      }
      rlecnt = 1;
    }
  }
  res[reslen] = 0;
  return(res);
}

/* normalize SDL keys to values easier to handle */
static int normalizekeys(SDL_Keycode key) {
  switch (key) {
    case SDLK_UP:
    case SDLK_KP_8:
      if (SDL_GetModState() & KMOD_CTRL) return(KEY_CTRL_UP);
      return(KEY_UP);
    case SDLK_DOWN:
    case SDLK_KP_2:
      if (SDL_GetModState() & KMOD_CTRL) return(KEY_CTRL_DOWN);
      return(KEY_DOWN);
    case SDLK_LEFT:
    case SDLK_KP_4:
      return(KEY_LEFT);
    case SDLK_RIGHT:
    case SDLK_KP_6:
      return(KEY_RIGHT);
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      if (SDL_GetModState() & KMOD_ALT) return(KEY_FULLSCREEN);
      return(KEY_ENTER);
    case SDLK_BACKSPACE:
      return(KEY_BACKSPACE);
    case SDLK_PAGEUP:
    case SDLK_KP_9:
      return(KEY_PAGEUP);
    case SDLK_PAGEDOWN:
    case SDLK_KP_3:
      return(KEY_PAGEDOWN);
    case SDLK_HOME:
    case SDLK_KP_7:
      return(KEY_HOME);
    case SDLK_END:
    case SDLK_KP_1:
      return(KEY_END);
    case SDLK_ESCAPE:
      return(KEY_ESCAPE);
    case SDLK_F1:
      return(KEY_F1);
    case SDLK_F2:
      return(KEY_F2);
    case SDLK_F3:
      return(KEY_F3);
    case SDLK_F4:
      return(KEY_F4);
    case SDLK_F5:
      return(KEY_F5);
    case SDLK_F6:
      return(KEY_F6);
    case SDLK_F7:
      return(KEY_F7);
    case SDLK_F8:
      return(KEY_F8);
    case SDLK_F9:
      return(KEY_F9);
    case SDLK_F10:
      return(KEY_F10);
    case SDLK_F11:
      return(KEY_FULLSCREEN);
    case SDLK_F12:
      return(KEY_F12);
    case SDLK_s:
      return(KEY_S);
    case SDLK_r:
      return(KEY_R);
    case SDLK_c:
      if (SDL_GetModState() & KMOD_CTRL) return(KEY_CTRL_C);
      break;
    case SDLK_v:
      if (SDL_GetModState() & KMOD_CTRL) return(KEY_CTRL_V);
      break;
  }
  return(KEY_UNKNOWN);
}


/* trims a trailing newline, if any, from a string */
static void trimstr(char *str) {
  int x, lastrealchar = -1;
  if (str == NULL) return;
  for (x = 0; str[x] != 0; x++) {
    switch (str[x]) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        break;
      default:
        lastrealchar = x;
        break;
    }
  }
  str[lastrealchar + 1] = 0;
}

/* returns 0 if string is not a legal solution. non-zero otherwise. */
static int isLegalSokoSolution(char *solstr) {
  if (solstr == NULL) return(0);
  if (strlen(solstr) < 1) return(0);
  for (;;) {
    switch (*solstr) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if (solstr[1] == 0) return(0); /* numbers are tolerated, but only if followed by something */
        break;
      case 'u':
      case 'U':
      case 'r':
      case 'R':
      case 'd':
      case 'D':
      case 'l':
      case 'L':
        break;
      case 0:
        return(1);
      default:
        return(0);
    }
    solstr += 1;
  }
}

/* a wrapper on the SDL_Delay() call. The difference is that it reminds the last call, so it knows if there is still time to wait or not. This allows to smooth out delays, providing accurate delaying across platforms.
 * Usage: first call it with a '0' parameter to initialize the timer, and then feed it with t microseconds as many times as needed. */
static int sokDelay(long t) {
  static Uint32 timetowait = 0, starttime = 0, irq = 0, irqfreq = 0;
  int res = 0;
  Uint32 curtime = 0;
  if (t <= 0) {
    starttime = SDL_GetTicks();
    timetowait = 0;
    irq = 0;
    irqfreq = (Uint32)(0 - t);
    t = 0;
  } else {
    timetowait += t;
    irq += t;
    if (irq >= irqfreq) {
      irq -= irqfreq;
      res = 1;
    }
  }
  for (;;) {
    curtime = SDL_GetTicks();
    if (starttime > curtime + 1000) break; /* the SDL_GetTicks() wraps after 49 days */
    if (curtime - starttime >= timetowait / 1000) break;
    SDL_Delay(1);
  }
  return(res);
}

static int flush_events(void) {
  SDL_Event event;
  int exitflag = 0;
  while (SDL_PollEvent(&event) != 0) if (event.type == SDL_QUIT) exitflag = 1;
  return(exitflag);
}

static void switchfullscreen(SDL_Window *window) {
  static int fullscreenflag = 0;
  fullscreenflag ^= 1;
  if (fullscreenflag != 0) {
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  } else {
    SDL_SetWindowFullscreen(window, 0);
  }
  SDL_Delay(50); /* wait for 50ms - the video thread needs some time to set things up */
  flush_events(); /* going fullscreen fires some garbage events that I don't want to hear about */
}


static int getoffseth(struct sokgame *game, int winw, unsigned short tilesize) {
  /* if playfield is smaller than the screen */
  if (game->field_width * tilesize <= winw) return((winw / 2) - (game->field_width * tilesize / 2));
  /* if playfield is larger than the screen */
  if (game->positionx * tilesize + (tilesize / 2) > (winw / 2)) {
    int res = (winw / 2) - (game->positionx * tilesize + (tilesize / 2));
    if ((game->field_width * tilesize) + res < winw) res = winw - (game->field_width * tilesize);
    return(res);
  }
  return(0);
}

static int getoffsetv(struct sokgame *game, int winh, int tilesize) {
  /* if playfield is smaller than the screen */
  if (game->field_height * tilesize <= winh) return((winh / 2) - (game->field_height * tilesize / 2));
  /* if playfield is larger than the screen */
  if (game->positiony * tilesize + (tilesize / 2) > winh / 2) {
    int res = (winh / 2) - (game->positiony * tilesize + (tilesize / 2));
    if ((game->field_height * tilesize) + res < winh) res = winh - (game->field_height * tilesize);
    return(res);
  }
  return(0);
}

/* wait for a key up to timeout seconds (-1 = indefinitely), while redrawing the renderer screen, if not null */
static int wait_for_a_key(int timeout, SDL_Renderer *renderer) {
  SDL_Event event;
  Uint32 timeouttime = SDL_GetTicks();
  if (timeout > 0) timeouttime += (Uint32)timeout * 1000;
  for (;;) {
    SDL_Delay(50);
    if (SDL_PollEvent(&event) != 0) {
      if (renderer != NULL) SDL_RenderPresent(renderer);
      if (event.type == SDL_QUIT) {
        return(1);
      } else if (event.type == SDL_KEYDOWN) {
        return(0);
      }
    }
    if ((timeout > 0) && (SDL_GetTicks() >= timeouttime)) return(0);
  }
}

/* display a bitmap onscreen */
static int displaytexture(SDL_Renderer *renderer, SDL_Texture *texture, SDL_Window *window, int timeout, int flags, unsigned char alpha) {
  int winw, winh;
  SDL_Rect rect, *rectptr;
  SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
  SDL_GetWindowSize(window, &winw, &winh);
  if (flags & DISPLAYCENTERED) {
    rectptr = &rect;
    rect.x = (winw - rect.w) / 2;
    rect.y = (winh - rect.h) / 2;
  } else {
    rectptr = NULL;
  }
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureAlphaMod(texture, alpha);
  if (SDL_RenderCopy(renderer, texture, NULL, rectptr) != 0) printf("SDL_RenderCopy() failed: %s\n", SDL_GetError());
  if ((flags & NOREFRESH) == 0) SDL_RenderPresent(renderer);
  if (timeout != 0) return(wait_for_a_key(timeout, renderer));
  return(0);
}

/* provides width and height of a string (in pixels) */
static void get_string_size(const char *string, int fontsize, const struct spritesstruct *sprites, int *w, int *h) {
  int glyphw, glyphh;
  *w = 0;
  *h = 0;
  while (*string != 0) {
    if (*string == ' ') {
      *w += FONT_SPACE_WIDTH * fontsize / 100;
    } else {
      SDL_QueryTexture(sprites->font[(unsigned char)(*string)], NULL, NULL, &glyphw, &glyphh);
      *w += glyphw * fontsize / 100 + FONT_KERNING * fontsize / 100;
      if (glyphh * fontsize / 100 > *h) *h = glyphh * fontsize / 100;
    }
    string += 1;
  }
}

/* explode a string into wordwrapped substrings */
static void wordwrap(const char *string, char **multiline, int maxlines, int maxwidth, int fontsize, const struct spritesstruct *sprites) {
  int lastspace, multilineid;
  int x, stringw, stringh;
  char *tmpstring;
  /* set all multiline entries to NULL */
  for (x = 0; x < maxlines; x++) multiline[x] = NULL;

  /* find the next word boundary */
  lastspace = -1;
  multilineid = 0;
  for (;;) { /* loop on every word, and check if we reached the end */
    for (x = lastspace + 1; ; x++) {
      if ((string[x] == ' ') || (string[x] == '\t') || (string[x] == '\n') || (string[x] == 0)) {
        lastspace = x;
        break;
      }
    }
    /* is this word boundary fitting on screen? */
    tmpstring = strdup(string);
    tmpstring[lastspace] = 0;
    get_string_size(tmpstring, fontsize, sprites, &stringw, &stringh);
    if (stringw < maxwidth) {
      if (multiline[multilineid] != NULL) free(multiline[multilineid]);
      multiline[multilineid] = tmpstring;
    } else {
      free(tmpstring);
      if (multiline[multilineid] == NULL) break;
      lastspace = -1;
      string += strlen(multiline[multilineid]) + 1;
      multilineid += 1;
      if (multilineid >= maxlines) {
        size_t lastlinelen = strlen(multiline[multilineid - 1]);
        /* if text have been truncated, print '...' at the end of the last line */
        if (lastlinelen >= 3) {
          multiline[multilineid - 1][lastlinelen - 3] = '.';
          multiline[multilineid - 1][lastlinelen - 2] = '.';
          multiline[multilineid - 1][lastlinelen - 1] = '.';
        }
        break;
      }
    }
    if ((lastspace >= 0) && (string[lastspace] == 0)) break;
  }
}

/* blits a string onscreen, scaling the font at fontsize percents. The string is placed at starting position x/y */
static void draw_string(const char *orgstring, int fontsize, unsigned char alpha, struct spritesstruct *sprites, SDL_Renderer *renderer, int x, int y, SDL_Window *window, int maxlines, int pheight) {
  int i, winw, winh;
  char *string;
  SDL_Texture *glyph;
  SDL_Rect rectsrc, rectdst;
  char *multiline[16];
  int multilineid = 0;
  if (maxlines > 16) maxlines = 16;
  /* get size of the window */
  SDL_GetWindowSize(window, &winw, &winh);
  wordwrap(orgstring, multiline, maxlines, winw - x, fontsize, sprites);
  /* loop on every line */
  for (multilineid = 0; (multilineid < maxlines) && (multiline[multilineid] != NULL); multilineid += 1) {
    string = multiline[multilineid];
    if (multilineid > 0) y += pheight;
    /* if centering is requested, get size of the string */
    if ((x < 0) || (y < 0)) {
      int stringw, stringh;
      /* get pixel length of the string */
      get_string_size(string, fontsize, sprites, &stringw, &stringh);
      if (x == DRAWSTRING_CENTER) x = (winw - stringw) >> 1;
      if (x == DRAWSTRING_RIGHT) x = winw - stringw - 10;
      if (y == DRAWSTRING_BOTTOM) y = winh - stringh;
      if (y == DRAWSTRING_CENTER) y = (winh - stringh) / 2;
    }
    rectdst.x = x;
    rectdst.y = y;
    for (i = 0; string[i] != 0; i++) {
      if (string[i] == ' ') {
        rectdst.x += FONT_SPACE_WIDTH * fontsize / 100;
        continue;
      }
      glyph = sprites->font[(unsigned char)(string[i])];
      SDL_QueryTexture(glyph, NULL, NULL, &rectsrc.w, &rectsrc.h);
      rectdst.w = rectsrc.w * fontsize / 100;
      rectdst.h = rectsrc.h * fontsize / 100;
      SDL_SetTextureAlphaMod(glyph, alpha);
      SDL_RenderCopy(renderer, glyph, NULL, &rectdst);
      rectdst.x += (rectsrc.w * fontsize / 100) + (FONT_KERNING * fontsize / 100);
    }
    /* free the multiline memory */
    free(string);
  }
}


static int wallcap_isneeded(struct sokgame *game, int x, int y, int corner) {
  switch (corner) {
    case 0: /* top left corner */
      if ((x > 0) && (y > 0) && (game->field[x - 1][y] & game->field[x][y - 1] & game->field[x - 1][y - 1] & field_wall)) return(1);
      break;
    case 1: /* top right corner */
      if ((y > 0) && (game->field[x + 1][y] & game->field[x][y - 1] & game->field[x + 1][y - 1] & field_wall)) return(1);
      break;
    case 2: /* bottom right corner */
      if ((game->field[x + 1][y] & game->field[x][y + 1] & game->field[x + 1][y + 1] & field_wall)) return(1);
      break;
    case 3: /* bottom left corner */
      if ((x > 0) && (game->field[x - 1][y] & game->field[x][y + 1] & game->field[x - 1][y + 1] & field_wall)) return(1);
      break;
  }
  return(0);
}


/* get an 'id' for a wall on a given position. this is a 4-bits bitfield that indicates where the wall has neighbors (up/right/down/left). */
static unsigned short getwallid(struct sokgame *game, int x, int y) {
  unsigned short res = 0;
  if ((y > 0) && (game->field[x][y - 1] & field_wall)) res |= 1;
  if ((x < 63) && (game->field[x + 1][y] & field_wall)) res |= 2;
  if ((y < 63) && (game->field[x][y + 1] & field_wall)) res |= 4;
  if ((x > 0) && (game->field[x - 1][y] & field_wall)) res |= 8;
  return(res);
}


static void draw_playfield_tile(struct sokgame *game, int x, int y, struct spritesstruct *sprites, SDL_Renderer *renderer, int winw, int winh, struct videosettings *settings, int flags, int moveoffsetx, int moveoffsety) {
  int xpix, ypix;
  /* compute the pixel coordinates of the destination field */
  xpix = getoffseth(game, winw, settings->tilesize) + (x * settings->tilesize) + moveoffsetx;
  ypix = getoffsetv(game, winh, settings->tilesize) + (y * settings->tilesize) + moveoffsety;

  if ((flags & DRAWPLAYFIELDTILE_DRAWATOM) == 0) {
    if (game->field[x][y] & field_floor) gra_rendertile(renderer, sprites, SPRITE_FLOOR, xpix, ypix, settings->tilesize, 0);
    if (game->field[x][y] & field_goal) gra_rendertile(renderer, sprites, SPRITE_GOAL, xpix, ypix, settings->tilesize, 0);
    if (game->field[x][y] & field_wall) {
      unsigned short i;
      gra_rendertile(renderer, sprites, SPRITE_WALL0 + getwallid(game, x, y), xpix, ypix, settings->tilesize, 0);
      /* draw the wall element (in 4 times, to draw caps when necessary) */
      for (i = 0; i < 4; i++) {
        if (wallcap_isneeded(game, x, y, i)) {
          gra_rendertile(renderer, sprites, SPRITE_WALLCR + i, xpix, ypix, settings->tilesize, 0);
        }
      }
    }
  } else if (game->field[x][y] & field_atom) {
    unsigned short boxsprite = SPRITE_BOX;
    if (game->field[x][y] & field_goal) {
      boxsprite = SPRITE_BOXOK;
      if (flags & DRAWPLAYFIELDTILE_PUSH) {
        if ((game->positionx == x - 1) && (game->positiony == y) && (moveoffsetx > 0) && ((game->field[x + 1][y] & field_goal) == 0)) boxsprite = SPRITE_BOX;
        if ((game->positionx == x + 1) && (game->positiony == y) && (moveoffsetx < 0) && ((game->field[x - 1][y] & field_goal) == 0)) boxsprite = SPRITE_BOX;
        if ((game->positionx == x) && (game->positiony == y - 1) && (moveoffsety > 0) && ((game->field[x][y + 1] & field_goal) == 0)) boxsprite = SPRITE_BOX;
        if ((game->positionx == x) && (game->positiony == y + 1) && (moveoffsety < 0) && ((game->field[x][y - 1] & field_goal) == 0)) boxsprite = SPRITE_BOX;
      }
    }
    gra_rendertile(renderer, sprites, boxsprite, xpix, ypix, settings->tilesize, 0);
  }
}

static void draw_player(struct sokgame *game, struct sokgamestates *states, struct spritesstruct *sprites, SDL_Renderer *renderer, int winw, int winh, const struct videosettings *settings, int offsetx, int offsety) {
  SDL_Rect rect;

  /* compute the dst rect */
  rect.x = getoffseth(game, winw, settings->tilesize) + (game->positionx * settings->tilesize) + offsetx;
  rect.y = getoffsetv(game, winh, settings->tilesize) + (game->positiony * settings->tilesize) + offsety;
  rect.w = settings->tilesize;
  rect.h = settings->tilesize;

  gra_rendertile(renderer, sprites, sprites->playerid, rect.x, rect.y, settings->tilesize, states->angle);
}


static void draw_screen(struct sokgame *game, struct sokgamestates *states, struct spritesstruct *sprites, SDL_Renderer *renderer, SDL_Window *window, struct videosettings *settings, int moveoffsetx, int moveoffsety, int scrolling, int flags, char *levelname) {
  int x, y, winw, winh, offx, offy;
  /* int partialoffsetx = 0, partialoffsety = 0; */
  char stringbuff[256];
  int scrollingadjx = 0, scrollingadjy = 0; /* this is used when scrolling + movement of player is needed */
  int drawtile_flags = 0;

  SDL_GetWindowSize(window, &winw, &winh);
  SDL_RenderClear(renderer);

  if ((flags & DRAWSCREEN_NOBG) == 0) {
    gra_renderbg(renderer, sprites, SPRITE_BG, settings->tilesize, winw, winh);
  }

  if (flags & DRAWSCREEN_PUSH) drawtile_flags = DRAWPLAYFIELDTILE_PUSH;

  if (scrolling > 0) {
    if (moveoffsetx > scrolling) {
      scrollingadjx = moveoffsetx - scrolling;
      moveoffsetx = scrolling;
    }
    if (moveoffsetx < -scrolling) {
      scrollingadjx = moveoffsetx + scrolling;
      moveoffsetx = -scrolling;
    }
    if (moveoffsety > scrolling) {
      scrollingadjy = moveoffsety - scrolling;
      moveoffsety = scrolling;
    }
    if (moveoffsety < -scrolling) {
      scrollingadjy = moveoffsety + scrolling;
      moveoffsety = -scrolling;
    }
  }
  /* draw non-moveable tiles (floors, walls, goals) */
  for (y = 0; y < game->field_height; y++) {
    for (x = 0; x < game->field_width; x++) {
      if (scrolling != 0) {
        draw_playfield_tile(game, x, y, sprites, renderer, winw, winh, settings, drawtile_flags, -moveoffsetx, -moveoffsety);
      } else {
        draw_playfield_tile(game, x, y, sprites, renderer, winw, winh, settings, drawtile_flags, 0, 0);
      }
    }
  }
  /* draw moveable elements (atoms) */
  for (y = 0; y < game->field_height; y++) {
    for (x = 0; x < game->field_width; x++) {
      offx = 0;
      offy = 0;
      if (scrolling == 0) {
        if ((moveoffsetx > 0) && (x == game->positionx + 1) && (y == game->positiony)) offx = moveoffsetx;
        if ((moveoffsetx < 0) && (x == game->positionx - 1) && (y == game->positiony)) offx = moveoffsetx;
        if ((moveoffsety > 0) && (y == game->positiony + 1) && (x == game->positionx)) offy = moveoffsety;
        if ((moveoffsety < 0) && (y == game->positiony - 1) && (x == game->positionx)) offy = moveoffsety;
      } else {
        offx = -moveoffsetx;
        offy = -moveoffsety;
        if ((moveoffsetx > 0) && (x == game->positionx + 1) && (y == game->positiony)) offx = scrollingadjx;
        if ((moveoffsetx < 0) && (x == game->positionx - 1) && (y == game->positiony)) offx = scrollingadjx;
        if ((moveoffsety > 0) && (y == game->positiony + 1) && (x == game->positionx)) offy = scrollingadjy;
        if ((moveoffsety < 0) && (y == game->positiony - 1) && (x == game->positionx)) offy = scrollingadjy;
      }
      draw_playfield_tile(game, x, y, sprites, renderer, winw, winh, settings, DRAWPLAYFIELDTILE_DRAWATOM, offx, offy);
    }
  }
  /* draw where the player is */
  if (scrolling != 0) {
    draw_player(game, states, sprites, renderer, winw, winh, settings, scrollingadjx, scrollingadjy);
  } else {
    draw_player(game, states, sprites, renderer, winw, winh, settings, moveoffsetx, moveoffsety);
  }
  /* draw text */
  if ((flags & DRAWSCREEN_NOTXT) == 0) {
    sprintf(stringbuff, "%s, level %d", levelname, game->level);
    draw_string(stringbuff, 100, 255, sprites, renderer, 10, DRAWSTRING_BOTTOM, window, 1, 0);
    if (game->solution != NULL) {
      sprintf(stringbuff, "best score: %lu/%lu", (unsigned long)sok_history_getlen(game->solution), (unsigned long)sok_history_getpushes(game->solution));
    } else {
      sprintf(stringbuff, "best score: -");
    }
    draw_string(stringbuff, 100, 255, sprites, renderer, DRAWSTRING_RIGHT, 0, window, 1, 0);
    sprintf(stringbuff, "moves: %lu / pushes: %lu", (unsigned long)sok_history_getlen(states->history), (unsigned long)sok_history_getpushes(states->history));
    draw_string(stringbuff, 100, 255, sprites, renderer, 10, 0, window, 1, 0);
  }
  if ((flags & DRAWSCREEN_PLAYBACK) && (time(NULL) % 2 == 0)) draw_string("*** PLAYBACK ***", 100, 255, sprites, renderer, DRAWSTRING_CENTER, 32, window, 1, 0);
  /* Update the screen */
  if (flags & DRAWSCREEN_REFRESH) SDL_RenderPresent(renderer);
}

static int rotatePlayer(struct spritesstruct *sprites, struct sokgame *game, struct sokgamestates *states, enum SOKMOVE dir, SDL_Renderer *renderer, SDL_Window *window, struct videosettings *settings, char *levelname, int drawscreenflags) {
  int srcangle = states->angle;
  int dstangle, dirmotion, winw, winh;
  SDL_GetWindowSize(window, &winw, &winh);
  switch (dir) {
    case sokmoveNONE:
    case sokmoveUP:
      dstangle = 0;
      break;
    case sokmoveRIGHT:
      dstangle = 90;
      break;
    case sokmoveDOWN:
      dstangle = 180;
      break;
    case sokmoveLEFT:
      dstangle = 270;
      break;
  }
  /* figure out how to compute the shortest way to rotate the player... This is not a very efficient way, but it works.. I might improve it in the future... */
  if (srcangle != dstangle) {
    int tmpangle, stepsright = 0, stepsleft = 0;
    for (tmpangle = srcangle; ; tmpangle += 90) {
      if (tmpangle >= 360) tmpangle -= 360;
      stepsright += 1;
      if (tmpangle == dstangle) break;
    }
    for (tmpangle = srcangle; ; tmpangle -= 90) {
      if (tmpangle < 0) tmpangle += 360;
      stepsleft += 1;
      if (tmpangle == dstangle) break;
    }
    if (stepsleft < stepsright) {
      dirmotion = -1;
    } else if (stepsleft > stepsright) {
      dirmotion = 1;
    } else {
      if (rand() % 2 == 0) {
        dirmotion = -1;
      } else {
        dirmotion = 1;
      }
    }
    /* perform the rotation */
    sokDelay(0 - settings->framefreq); /* init my delay timer */
    for (tmpangle = srcangle; ; tmpangle += dirmotion) {
      if (tmpangle >= 360) tmpangle = 0;
      if (tmpangle < 0) tmpangle = 359;
      states->angle = tmpangle;
      if (sokDelay(settings->framedelay / 8)) { /* wait for x ms */
        draw_screen(game, states, sprites, renderer, window, settings, 0, 0, 0, DRAWSCREEN_REFRESH | drawscreenflags, levelname);
      }
      if (tmpangle == dstangle) break;
    }
    return(1);
  }
  return(0);
}

static int scrollneeded(struct sokgame *game, SDL_Window *window, unsigned short tilesize, int offx, int offy) {
  int winw, winh, offsetx, offsety, result = 0;
  SDL_GetWindowSize(window, &winw, &winh);
  offsetx = absval(getoffseth(game, winw, tilesize));
  offsety = absval(getoffsetv(game, winh, tilesize));
  game->positionx += offx;
  game->positiony += offy;
  result = offsetx - absval(getoffseth(game, winw, tilesize));
  if (result == 0) result = offsety - absval(getoffsetv(game, winh, tilesize));
  if (result < 0) result = -result; /* convert to abs() value */
  game->positionx -= offx;
  game->positiony -= offy;
  return(result);
}

static void loadlevel(struct sokgame *togame, struct sokgame *fromgame, struct sokgamestates *states) {
  memcpy(togame, fromgame, sizeof(struct sokgame));
  sok_resetstates(states);
}

static char *processDropFileEvent(SDL_Event *event, char **levelfile) {
  if (event->type != SDL_DROPFILE) return(NULL);
  if (event->drop.file == NULL) return(NULL);
  if (*levelfile != NULL) free(*levelfile);
  *levelfile = strdup(event->drop.file);
  return(*levelfile);
}

/* waits for the user to choose a game type or to load an external xsb file and returns either a pointer to a memory chunk with xsb data or to fill levelfile with a filename */
static unsigned char *selectgametype(SDL_Renderer *renderer, struct spritesstruct *sprites, SDL_Window *window, struct videosettings *settings, char **levelfile, size_t *levelfilelen) {
  int winw, winh, stringw, stringh, longeststringw;
  static int selection = 0;
  int oldpusherposy = 0, newpusherposy, x, selectionchangeflag = 0;
  unsigned char *memptr[3];
  size_t memptrlen[3];
  char *levname[5] = {"Easy (Microban)", "Normal (Sasquatch)", "Hard (Sasquatch III)", "Internet levels", "Quit"};
  int textvadj = 12;
  int selectionpos[16];
  SDL_Event event;
  SDL_Rect rect;

  *levelfilelen = 0;
  memptr[0] = assets_levels_microban_xsb_gz;
  memptr[1] = assets_levels_sasquatch_xsb_gz;
  memptr[2] = assets_levels_sasquatch3_xsb_gz;
  memptrlen[0] = assets_levels_microban_xsb_gz_len;
  memptrlen[1] = assets_levels_sasquatch_xsb_gz_len;
  memptrlen[2] = assets_levels_sasquatch3_xsb_gz_len;

  /* compute the pixel width of the longest string in the menu */
  longeststringw = 0;
  for (x = 0; x < 5; x++) {
    get_string_size(levname[x], 100, sprites, &stringw, &stringh);
    if (stringw > longeststringw) longeststringw = stringw;
  }

  for (;;) {
    int refreshnow = 1;
    /* get windows x / y size */
    SDL_GetWindowSize(window, &winw, &winh);

    /* precompute the y-axis position of every line */
    for (x = 0; x < 5; x++) {
      selectionpos[x] = (int)((winh * 0.51) + (winh * 0.06 * x));
      if (x > 2) selectionpos[x] += (winh / 64); /* add a small vertical gap before "Internet levels" */
      if (x > 3) selectionpos[x] += (winh / 64); /* add a small vertical gap before "Quit" */
    }

    /* compute the dst rect of the pusher */
    rect.w = settings->tilesize;
    rect.h = settings->tilesize;
    rect.x = ((winw - longeststringw) >> 1) - 54;
    newpusherposy = selectionpos[selection] + 25 - (rect.h / 2);
    if (selectionchangeflag == 0) oldpusherposy = newpusherposy;
    /* draw the screen */
    rect.y = oldpusherposy;
    sokDelay(0 - settings->framefreq); /* init my delay timer */
    for (;;) {
      if (refreshnow) { /* wait for x ms */
        SDL_RenderClear(renderer);
        gra_renderbg(renderer, sprites, SPRITE_BG, settings->tilesize, winw, winh);
        { /* render title, version and copyright string */
          int sokow, sokoh, simpw, simph, verw, verh, copyw, copyh;
          int tity;
          const char *simpstr = "simple";
          const char *sokostr = "SOKOBAN";
          const char *verstr = "ver " PVER;
          const char *copystr = "Copyright (C) " PDATE " Mateusz Viste";

          get_string_size(simpstr, 100, sprites, &simpw, &simph);
          get_string_size(sokostr, 300, sprites, &sokow, &sokoh);
          get_string_size(verstr, 100, sprites, &verw, &verh);
          get_string_size(copystr, 60, sprites, &copyw, &copyh);

          tity = (selectionpos[0] - (sokoh * 8 / 10)) / 2 - (simph * 8 / 10);

          draw_string(simpstr, 100, 200, sprites, renderer, 10 + (winw - sokow) / 2, tity, window, 1, 0);
          tity += simph * 8 / 10;
          draw_string(sokostr, 300, 255, sprites, renderer, (winw - sokow) / 2, tity, window, 1, 0);
          tity += sokoh * 8 / 10;
          draw_string(verstr, 100, 180, sprites, renderer, (sokow + (winw - sokow) / 2) - verw, tity, window, 1, 0);

          /* copyright string */
          draw_string(copystr, 60, 200, sprites, renderer, winw - (copyw + 5), winh - copyh, window, 1, 0);
        }

        gra_rendertile(renderer, sprites, sprites->playerid, rect.x, rect.y, settings->tilesize, 90);
        for (x = 0; x < 5; x++) {
          draw_string(levname[x], 100, 255, sprites, renderer, rect.x + 54, textvadj + selectionpos[x], window, 1, 0);
        }
        SDL_RenderPresent(renderer);
        if (rect.y == newpusherposy) break;
      }
      if (newpusherposy < oldpusherposy) {
        rect.y -= 1;
        if (rect.y < newpusherposy) rect.y = newpusherposy;
      } else {
        rect.y += 1;
        if (rect.y > newpusherposy) rect.y = newpusherposy;
      }
      refreshnow = sokDelay(settings->framedelay / 4);
    }
    oldpusherposy = newpusherposy;
    selectionchangeflag = 0;

    /* Wait for an event - but ignore 'KEYUP' and 'MOUSEMOTION' events, since they are worthless in this game */
    for (;;) if ((SDL_WaitEvent(&event) != 0) && (event.type != SDL_KEYUP) && (event.type != SDL_MOUSEMOTION)) break;

    /* check what event we got */
    if (event.type == SDL_QUIT) {
      return(NULL);
    } else if (event.type == SDL_DROPFILE) {
      if (processDropFileEvent(&event, levelfile) != NULL) return(NULL);
    } else if (event.type == SDL_KEYDOWN) {
      switch (normalizekeys(event.key.keysym.sym)) {
        case KEY_UP:
          selection--;
          selectionchangeflag = 1;
          break;
        case KEY_DOWN:
          selection++;
          selectionchangeflag = 1;
          break;
        case KEY_ENTER:
          if (selection == 3) return((unsigned char *)"@");
          if (selection == 4) return(NULL); /* Quit */
          *levelfilelen = memptrlen[selection];
          return(memptr[selection]);
        case KEY_FULLSCREEN:
          switchfullscreen(window);
          break;
        case KEY_ESCAPE:
          return(NULL);
      }
      if (selection < 0) selection = 4;
      if (selection > 4) selection = 0;
    }
  }
}


/* blit a level preview */
static void blit_levelmap(struct sokgame *game, struct spritesstruct *sprites, int xpos, int ypos, SDL_Renderer *renderer, unsigned short tilesize, unsigned char alpha, int flags) {
  int x, y, bgpadding = tilesize * 3;
  SDL_Rect rect, bgrect;

  bgrect.x = xpos - (game->field_width * tilesize + bgpadding) / 2;
  bgrect.y = ypos - (game->field_height * tilesize + bgpadding) / 2;
  bgrect.w = game->field_width * tilesize + bgpadding;
  bgrect.h = game->field_height * tilesize + bgpadding;
  /* if background enabled, compute coordinates of the background and draw it */
  if (flags & BLIT_LEVELMAP_BACKGROUND) {
    SDL_SetRenderDrawColor(renderer, 0x12, 0x12, 0x12, 255);
    SDL_RenderFillRect(renderer, &bgrect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  }
  for (y = 0; y < game->field_height; y++) {
    for (x = 0; x < game->field_width; x++) {
      /* compute coordinates of the tile on screen */
      rect.x = xpos + (tilesize * x) - (game->field_width * tilesize) / 2;
      rect.y = ypos + (tilesize * y) - (game->field_height * tilesize) / 2;
      /* draw the tile */
      if (game->field[x][y] & field_floor) gra_rendertile(renderer, sprites, SPRITE_FLOOR, rect.x, rect.y, tilesize, 0);
      if (game->field[x][y] & field_wall) {
        unsigned short i;
        gra_rendertile(renderer, sprites, SPRITE_WALL0 + getwallid(game, x, y), rect.x, rect.y, tilesize, 0);
        /* check for neighbors and draw wall cap if needed */
        for (i = 0; i < 4; i++) {
          if (wallcap_isneeded(game, x, y, i)) {
            gra_rendertile(renderer, sprites, SPRITE_WALLCR + i, rect.x, rect.y, tilesize, 0);
          }
        }
      }
      if ((game->field[x][y] & field_goal) && (game->field[x][y] & field_atom)) { /* atom on goal */
        gra_rendertile(renderer, sprites, SPRITE_BOXOK, rect.x, rect.y, tilesize, 0);
      } else if (game->field[x][y] & field_goal) { /* goal */
        gra_rendertile(renderer, sprites, SPRITE_GOAL, rect.x, rect.y, tilesize, 0);
      } else if (game->field[x][y] & field_atom) { /* atom */
        gra_rendertile(renderer, sprites, SPRITE_BOX, rect.x, rect.y, tilesize, 0);
      }
    }
  }
  /* apply alpha filter */
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255 - alpha);
  SDL_RenderFillRect(renderer, &bgrect);
  /* if background enabled, then draw the border */
  if (flags & BLIT_LEVELMAP_BACKGROUND) {
    unsigned char fadealpha;
    SDL_SetRenderDrawColor(renderer, 0x28, 0x28, 0x28, 255);
    SDL_RenderDrawRect(renderer, &bgrect);
    /* draw a nice fade-out effect around the selected level */
    for (fadealpha = 1; fadealpha < 20; fadealpha++) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255 - fadealpha * (255 / 20));
      bgrect.x -= 1;
      bgrect.y -= 1;
      bgrect.w += 2;
      bgrect.h += 2;
      SDL_RenderDrawRect(renderer, &bgrect);
    }
    /* set the drawing color to its default, plain black color */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  }
  /* if level is solved, draw a 'complete' tag */
  if (game->solution != NULL) {
    /* SDL_Rect rect; */
    SDL_QueryTexture(sprites->solved, NULL, NULL, &rect.w, &rect.h);
    rect.w = rect.w * sprites->em / 60;
    rect.h = rect.h * sprites->em / 60;
    rect.x = xpos - (rect.w / 2);
    rect.y = ypos - (rect.h * 3 / 4);
    SDL_RenderCopy(renderer, sprites->solved, NULL, &rect);
  }
}

static int fade2texture(SDL_Renderer *renderer, SDL_Window *window, SDL_Texture *texture) {
  int exitflag = 0;
  unsigned char alphaval;
  sokDelay(0);  /* init my delay timer */
  for (alphaval = 0; alphaval < 64; alphaval += 4) {
    exitflag = displaytexture(renderer, texture, window, 0, 0, alphaval);
    if (exitflag != 0) break;
    sokDelay(15 * 1000);  /* wait for 15ms */
  }
  if (exitflag == 0) exitflag = displaytexture(renderer, texture, window, 0, 0, 255);
  return(exitflag);
}

static int selectlevel(struct sokgame **gameslist, struct spritesstruct *sprites, SDL_Renderer *renderer, SDL_Window *window, struct videosettings *settings, char *levcomment, int levelscount, int selection, char **levelfile) {
  int i, winw, winh, maxallowedlevel;
  char levelnum[64];
  SDL_Event event;
  /* reload all solutions for levels, in case they changed (for ex. because we just solved a level..) */
  sok_loadsolutions(gameslist, levelscount);

  /* if no current level is selected, then preselect the first unsolved level */
  if (selection < 0) {
    for (i = 0; i < levelscount; i++) {
      if (gameslist[i]->solution != NULL) {
        if (debugmode != 0) printf("Level %d [%08lX] has solution: %s\n", i + 1, gameslist[i]->crc32, gameslist[i]->solution);
      } else {
        if (debugmode != 0) printf("Level %d [%08lX] has NO solution\n", i + 1, gameslist[i]->crc32);
        selection = i;
        break;
      }
    }
  }

  /* if no unsolved level found, then select the first one */
  if (selection < 0) selection = 0;

  /* compute the last allowed level */
  i = 0; /* i will temporarily store the number of unsolved levels */
  for (maxallowedlevel = 0; maxallowedlevel < levelscount; maxallowedlevel++) {
    if (gameslist[maxallowedlevel]->solution == NULL) i++;
    if (i > 3) break; /* user can see up to 3 unsolved levels */
  }

  /* loop */
  for (;;) {
    SDL_GetWindowSize(window, &winw, &winh);

    /* draw the screen */
    SDL_RenderClear(renderer);
    /* draw the level before */
    if (selection > 0) blit_levelmap(gameslist[selection - 1], sprites, winw / 5, winh / 2, renderer, settings->tilesize / 4, 96, 0);
    /* draw the level after */
    if (selection + 1 < maxallowedlevel) blit_levelmap(gameslist[selection + 1], sprites, winw * 4 / 5,  winh / 2, renderer, settings->tilesize / 4, 96, 0);
    /* draw the selected level */
    blit_levelmap(gameslist[selection], sprites,  winw / 2,  winh / 2, renderer, settings->tilesize / 3, 210, BLIT_LEVELMAP_BACKGROUND);
    /* draw strings, etc */
    draw_string(levcomment, 100, 255, sprites, renderer, DRAWSTRING_CENTER, winh / 8, window, 1, 0);
    draw_string("(choose a level)", 100, 255, sprites, renderer, DRAWSTRING_CENTER, winh / 8 + 40, window, 1, 0);
    sprintf(levelnum, "Level %d of %d", selection + 1, levelscount);
    draw_string(levelnum, 100, 255, sprites, renderer, DRAWSTRING_CENTER, winh * 3 / 4, window, 1, 0);
    SDL_RenderPresent(renderer);

    /* Wait for an event - but ignore 'KEYUP' and 'MOUSEMOTION' events, since they are worthless in this game */
    for (;;) if ((SDL_WaitEvent(&event) != 0) && (event.type != SDL_KEYUP) && (event.type != SDL_MOUSEMOTION)) break;

    /* check what event we got */
    if (event.type == SDL_QUIT) {
      return(SELECTLEVEL_QUIT);
    } else if (event.type == SDL_DROPFILE) {
      if (processDropFileEvent(&event, levelfile) != NULL) {
        fade2texture(renderer, window, sprites->black);
        return(SELECTLEVEL_LOADFILE);
      }
    } else if (event.type == SDL_KEYDOWN) {
      switch (normalizekeys(event.key.keysym.sym)) {
        case KEY_LEFT:
          if (selection > 0) selection--;
          break;
        case KEY_RIGHT:
          if (selection + 1 < maxallowedlevel) selection++;
          break;
        case KEY_HOME:
          selection = 0;
          break;
        case KEY_END:
          selection = maxallowedlevel - 1;
          break;
        case KEY_PAGEUP:
          if (selection < 3) {
            selection = 0;
          } else {
            selection -= 3;
          }
          break;
        case KEY_PAGEDOWN:
          if (selection + 3 >= maxallowedlevel) {
            selection = maxallowedlevel - 1;
          } else {
            selection += 3;
          }
          break;
        case KEY_CTRL_UP:
          if (settings->tilesize < 255) settings->tilesize += 4;
          break;
        case KEY_CTRL_DOWN:
          if (settings->tilesize > 6) settings->tilesize -= 4;
          break;
        case KEY_ENTER:
          return(selection);
        case KEY_FULLSCREEN:
          switchfullscreen(window);
          break;
        case KEY_ESCAPE:
          fade2texture(renderer, window, sprites->black);
          return(SELECTLEVEL_BACK);
      }
    }
  }
}

/* sets the icon in the aplication's title bar */
static void setsokicon(SDL_Window *window) {
  SDL_Surface *surface;
  surface = loadgzbmp(assets_icon_bmp_gz, assets_icon_bmp_gz_len);
  if (surface == NULL) return;
  SDL_SetWindowIcon(window, surface);
  SDL_FreeSurface(surface); /* once the icon is loaded, the surface is not needed anymore */
}

/* returns 1 if curlevel is the last level to solve in the set. returns 0 otherwise. */
static int islevelthelastleft(struct sokgame **gamelist, int curlevel, int levelscount) {
  int x;
  if (curlevel < 0) return(0);
  if (gamelist[curlevel]->solution != NULL) return(0);
  for (x = 0; x < levelscount; x++) {
    if ((gamelist[x]->solution == NULL) && (x != curlevel)) return(0);
  }
  return(1);
}

static void dumplevel2clipboard(struct sokgame *game, char *history) {
  char *txt;
  unsigned long solutionlen = 0, playfieldsize;
  int x, y;
  if (game->solution != NULL) solutionlen = strlen(game->solution);
  playfieldsize = (game->field_width + 1) * game->field_height;
  txt = malloc(solutionlen + playfieldsize + 4096);
  if (txt == NULL) return;
  sprintf(txt, "; Level id: %lX\n\n", game->crc32);
  for (y = 0; y < game->field_height; y++) {
    for (x = 0; x < game->field_width; x++) {
      switch (game->field[x][y] & ~field_floor) {
        case field_wall:
          strcat(txt, "#");
          break;
        case (field_atom | field_goal):
          strcat(txt, "*");
          break;
        case field_atom:
          strcat(txt, "$");
          break;
        case field_goal:
          if ((game->positionx == x) && (game->positiony == y)) {
            strcat(txt, "+");
          } else {
            strcat(txt, ".");
          }
          break;
        default:
          if ((game->positionx == x) && (game->positiony == y)) {
            strcat(txt, "@");
          } else {
            strcat(txt, " ");
          }
          break;
      }
    }
    strcat(txt, "\n");
  }
  strcat(txt, "\n");
  if ((history != NULL) && (history[0] != 0)) { /* only allow if there actually is a solution */
    strcat(txt, "; Solution\n; ");
    strcat(txt, history);
    strcat(txt, "\n");
  } else {
    strcat(txt, "; No solution available\n");
  }
  SDL_SetClipboardText(txt);
  free(txt);
}

/* reads a chunk of text from memory. returns the line in a chunk of memory that needs to be freed afterwards */
static char *readmemline(char **memptrptr) {
  unsigned long linelen;
  char *res;
  char *memptr;
  memptr = *memptrptr;
  if (*memptr == 0) return(NULL);
  /* check how long the line is */
  for (linelen = 0; ; linelen += 1) {
    if (memptr[linelen] == 0) break;
    if (memptr[linelen] == '\n') break;
  }
  /* allocate memory for the line, and copy its content */
  res = malloc(linelen + 1);
  memcpy(res, memptr, linelen);
  /* move the original pointer forward */
  *memptrptr += linelen;
  if (**memptrptr == '\n') *memptrptr += 1;
  /* trim out trailing \r, if any */
  if ((linelen > 0) && (res[linelen - 1] == '\r')) linelen -= 1;
  /* terminate the line with a null terminator */
  res[linelen] = 0;
  return(res);
}

static void fetchtoken(char *res, char *buf, int pos) {
  int tokpos = 0, x;
  /* forward to the position where the token starts */
  while (tokpos != pos) {
    if (*buf == 0) {
      break;
    } else if (*buf == '\t') {
      tokpos += 1;
    }
    buf += 1;
  }
  /* copy the token to buf, until \t or \0 */
  for (x = 0;; x++) {
    if (buf[x] == '\0') break;
    if (buf[x] == '\t') break;
    res[x] = buf[x];
  }
  res[x] = 0;
}


static int selectinternetlevel(SDL_Renderer *renderer, SDL_Window *window, struct spritesstruct *sprites, char *host, unsigned short port, char *path, char *levelslist, unsigned char **xsbptr, size_t *reslen) {
  unsigned char *res = NULL;
  char url[2048], buff[1200], buff2[1024];
  char *inetlist[1024];
  int inetlistlen = 0, i, selected = 0, windowrows, fontheight = 24, winw, winh;
  static int selection = 0, seloffset = 0;
  SDL_Event event;
  *xsbptr = NULL;
  *reslen = 0;
  /* load levelslist into an array */
  for (;;) {
    inetlist[inetlistlen] = readmemline(&levelslist);
    if (inetlist[inetlistlen] == NULL) break;
    inetlistlen += 1;
    if (inetlistlen >= 1024) break;
  }
  if (inetlistlen < 1) return(SELECTLEVEL_BACK); /* if failed to load any level, quit here */
  /* selection loop */
  for (;;) {
    SDL_Rect rect;
    /* compute the amount of rows we can fit onscreen */
    SDL_GetWindowSize(window, &winw, &winh);
    windowrows = (winh / fontheight) - 7;
    /* display the list of levels */
    SDL_RenderClear(renderer);
    for (i = 0; i < windowrows; i++) {
      if (i + seloffset >= inetlistlen) break;
      fetchtoken(buff, inetlist[i + seloffset], 1);
      draw_string(buff, 100, 255, sprites, renderer, 30, i * fontheight, window, 1, 0);
      if (i + seloffset == selection) {
        gra_rendertile(renderer, sprites, sprites->playerid, 0, i * fontheight, 30, 90);
      }
    }
    /* render background of level description */
    rect.x = 0;
    rect.y = (windowrows * fontheight) + (fontheight * 4 / 10);
    rect.w = winw;
    rect.h = winh;
    SDL_SetRenderDrawColor(renderer, 0x30, 0x30, 0x30, 255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 0xC0, 0xC0, 0xC0, 255);
    SDL_RenderDrawLine(renderer, 0, rect.y, winw, rect.y);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    /* draw level description */
    rect.y += fontheight / 2;
    fetchtoken(buff2, inetlist[selection], 1);
    draw_string(buff2, 100, 250, sprites, renderer, DRAWSTRING_CENTER, rect.y, window, 1, 0);
    fetchtoken(buff2, inetlist[selection], 2);
    sprintf(buff, "Copyright (C) %s", buff2);
    draw_string(buff, 65, 200, sprites, renderer, DRAWSTRING_CENTER, rect.y + (fontheight * 12 / 10), window, 1, 0);
    fetchtoken(buff, inetlist[selection], 3);
    draw_string(buff, 100, 210, sprites, renderer, 0, rect.y + (fontheight * 26 / 10), window, 3, fontheight);
    /* refresh screen */
    SDL_RenderPresent(renderer);
    /* Wait for an event - but ignore 'KEYUP' and 'MOUSEMOTION' events, since they are worthless in this game */
    for (;;) {
      SDL_WaitEvent(&event);
      if ((event.type != SDL_KEYUP) && (event.type != SDL_MOUSEMOTION)) break;
    }
    /* check what event we got */
    if (event.type == SDL_QUIT) {
        selected = SELECTLEVEL_QUIT;
      /* } else if (event.type == SDL_DROPFILE) {
        if (processDropFileEvent(&event, &levelfile) != NULL) {
          fade2texture(renderer, window, sprites->black);
          goto GametypeSelectMenu;
        } */
      } else if (event.type == SDL_KEYDOWN) {
        switch (normalizekeys(event.key.keysym.sym)) {
          case KEY_UP:
            if (selection > 0) selection -= 1;
            if ((seloffset > 0) && (selection < seloffset + 2)) seloffset -= 1;
            break;
          case KEY_DOWN:
            if (selection + 1 < inetlistlen) selection += 1;
            if ((seloffset < inetlistlen - windowrows) && (selection >= seloffset + windowrows - 2)) seloffset += 1;
            break;
          case KEY_ENTER:
            selected = SELECTLEVEL_OK;
            break;
          case KEY_ESCAPE:
            selected = SELECTLEVEL_BACK;
            break;
          case KEY_FULLSCREEN:
            switchfullscreen(window);
            break;
          case KEY_HOME:
            selection = 0;
            seloffset = 0;
            break;
          case KEY_END:
            selection = inetlistlen - 1;
            seloffset = inetlistlen - windowrows;
            break;
        }
    }
    if (selected != 0) break;
  }
  /* fetch the selected level */
  if (selected == SELECTLEVEL_OK) {
    fetchtoken(buff, inetlist[selection], 0);
    sprintf(url, "%s%s", path, buff);
    *reslen = http_get(host, port, url, &res);
    *xsbptr = res;
  } else {
    *xsbptr = NULL;
  }
  /* free the list */
  while (inetlistlen > 0) {
    inetlistlen -= 1;
    free(inetlist[inetlistlen]);
  }
  fade2texture(renderer, window, sprites->black);
  return(selected);
}


/* returns a tilesize that is more or less consistent with the in-game fonts */
static unsigned short auto_tilesize(struct spritesstruct *spr) {
  unsigned short tilesize;

  tilesize = (spr->em + 1) * 3 / 2;

  /* drop the lowest bit so tilesize is guaranteed to be even */
  tilesize >>= 1;
  tilesize <<= 1;
  /* add a bit if em native tilesize is odd, this to make sure that the user
   * is able to zoom in/out to the native tile resolution (zooming is
   * performed by increments of 2) */
  tilesize |= (spr->tilesize & 1);

  return(tilesize);
}


static void list_installed_skins(void) {
  struct skinlist *list, *node;
  puts("List of installed skins:");
  list = skin_list();
  if (list == NULL) puts("no skins found");
  for (node = list; node != NULL; node = node->next) {
    printf("%-16s (%s)\n", node->name, node->path);
  }
  skin_list_free(list);
}


static int parse_cmdline(struct videosettings *settings, int argc, char **argv, char **levelfile) {
  /* pre-set a few default settings */
  memset(settings, 0, sizeof(*settings));
  settings->framedelay = -1;
  settings->framefreq = -1;
  settings->customskinfile = DEFAULT_SKIN;

  /* parse the commandline */
  if (argc > 1) {
    int i;
    for (i = 1 ; i < argc ; i++) {
      if (strstr(argv[i], "--framedelay=") == argv[i]) {
        settings->framedelay = atoi(argv[i] + strlen("--framedelay="));
      } else if (strstr(argv[i], "--framefreq=") == argv[i]) {
        settings->framefreq = atoi(argv[i] + strlen("--framefreq="));
      } else if (strstr(argv[i], "--skin=") == argv[i]) {
        settings->customskinfile = argv[i] + strlen("--skin=");
      } else if (strcmp(argv[i], "--skinlist") == 0) {
        list_installed_skins();
        return(1);
      } else if ((*levelfile == NULL) && (argv[i][0] != '-')) { /* else assume it is a level file */
        *levelfile = strdup(argv[i]);
      } else { /* invalid argument */
        puts("Simple Sokoban ver " PVER);
        puts("Copyright (C) " PDATE " Mateusz Viste");
        puts("");
        puts("usage: simplesok [options] [levelfile.xsb]");
        puts("");
        puts("options:");
        puts("  --framedelay=t      (microseconds)");
        puts("  --framefreq=t       (microseconds)");
        puts("  --skin=name         skin name to be used (default: antique3)");
        puts("  --skinlist          display the list of installed skins");
        puts("");
        puts("Skin files can be are stored in a couple of different directories:");
        puts(" * a skins/ subdirectory in SimpleSok's application directory");
        puts(" * /usr/share/simplesok/skins/");
        puts(" * a skins/ subdirectory in SimpleSok's user directory");
        puts("");
        puts("If skin loading fails, then a default (embedded) skin is used.");
        puts("");
        puts("homepage: http://simplesok.sourceforge.net");
        return(1);
      }
    }
  }
  return(0);
}


int main(int argc, char **argv) {
  struct sokgame **gameslist, game;
  struct sokgamestates *states;
  struct spritesstruct *sprites;
  int levelscount, curlevel, exitflag = 0, showhelp = 0, lastlevelleft = 0;
  int playsolution, drawscreenflags;
  char *levelfile = NULL;
  char *playsource = NULL;
  char *levelslist = NULL;
  #define LEVCOMMENTMAXLEN 32
  char levcomment[LEVCOMMENTMAXLEN];
  struct videosettings settings;
  unsigned char *xsblevelptr = NULL;
  size_t xsblevelptrlen = 0;
  enum leveltype levelsource = LEVEL_INTERNAL;

  SDL_Window* window = NULL;
  SDL_Renderer *renderer;
  SDL_Event event;

  /* init (seed) the randomizer */
  srand((unsigned int)time(NULL));

  exitflag = parse_cmdline(&settings, argc, argv, &levelfile);
  if (exitflag != 0) return(1);

  /* init networking stack (required on windows) */
  init_net();

  /* Init SDL and set the video mode */
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("SDL_Init() failed: %s\n", SDL_GetError());
    return(1);
  }
  /* set SDL scaling algorithm to "nearest neighbor" so there is no bleeding
   * of textures. Possible values are:
   * "0" or "nearest"  - nearest pixel sampling
   * "1" or "linear"   - linear filtering
   * "2" or "best"     - anisotropic filtering */
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

  window = SDL_CreateWindow("Simple Sokoban " PVER, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_DEFAULT_WIDTH, SCREEN_DEFAULT_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (window == NULL) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    return(1);
  }

  setsokicon(window);
  SDL_SetWindowMinimumSize(window, 160, 120);

  renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == NULL) {
    SDL_DestroyWindow(window);
    printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    return(1);
  }

  /* Load sprites */
  sprites = skin_load(settings.customskinfile, renderer);
  if (sprites == NULL) return(1);
  printf("loaded skin appears to have tiles %d pixels wide\n", sprites->tilesize);

  /* Hide the mouse cursor, disable mouse events and make sure DropEvents are enabled (sometimes they are not) */
  SDL_ShowCursor(SDL_DISABLE);
  SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

  /* validate parameters */
  if ((settings.framedelay < 0) || (settings.framedelay > 64000)) settings.framedelay = 10500;
  if ((settings.framefreq < 1) || (settings.framefreq > 1000000)) settings.framefreq = 15000;

  gameslist = malloc(sizeof(struct sokgame *) * MAXLEVELS);
  if (gameslist == NULL) {
    puts("Memory allocation failed!");
    return(1);
  }

  states = sok_newstates();
  if (states == NULL) return(1);

  GametypeSelectMenu:
  if (levelslist != NULL) {
    free(levelslist);
    levelslist = NULL;
  }
  curlevel = -1;
  levelscount = -1;
  settings.tilesize = auto_tilesize(sprites);
  if (levelfile != NULL) goto LoadLevelFile;
  xsblevelptr = selectgametype(renderer, sprites, window, &settings, &levelfile, &xsblevelptrlen);
  levelsource = LEVEL_INTERNAL;
  if ((xsblevelptr != NULL) && (*xsblevelptr == '@')) levelsource = LEVEL_INTERNET;
  if (exitflag == 0) fade2texture(renderer, window, sprites->black);

  LoadInternetLevels:
  if (levelsource == LEVEL_INTERNET) { /* internet levels */
    int selectres;
    size_t httpres;
    httpres = http_get(INET_HOST, INET_PORT, INET_PATH, (unsigned char **) &levelslist);
    if ((httpres == 0) || (levelslist == NULL)) {
      SDL_RenderClear(renderer);
      draw_string("Failed to fetch internet levels!", 100, 255, sprites, renderer, DRAWSTRING_CENTER, DRAWSTRING_CENTER, window, 1, 0);
      wait_for_a_key(-1, renderer);
      goto GametypeSelectMenu;
    }
    selectres = selectinternetlevel(renderer, window, sprites, INET_HOST, INET_PORT, INET_PATH, levelslist, &xsblevelptr, &xsblevelptrlen);
    if (selectres == SELECTLEVEL_BACK) goto GametypeSelectMenu;
    if (selectres == SELECTLEVEL_QUIT) exitflag = 1;
    if (exitflag == 0) fade2texture(renderer, window, sprites->black);
  } else if ((xsblevelptr == NULL) && (levelfile == NULL)) { /* nothing */
    exitflag = 1;
  }

  LoadLevelFile:
  if ((levelfile != NULL) && (exitflag == 0)) {
    levelscount = sok_loadfile(gameslist, MAXLEVELS, levelfile, NULL, 0, levcomment, LEVCOMMENTMAXLEN);
  } else if (exitflag == 0) {
    levelscount = sok_loadfile(gameslist, MAXLEVELS, NULL, xsblevelptr, xsblevelptrlen, levcomment, LEVCOMMENTMAXLEN);
  }

  if ((levelscount < 1) && (exitflag == 0)) {
    SDL_RenderClear(renderer);
    printf("Failed to load the level file [%d]: %s\n", levelscount, sok_strerr(levelscount));
    draw_string("Failed to load the level file!", 100, 255, sprites, renderer, DRAWSTRING_CENTER, DRAWSTRING_CENTER, window, 1, 0);
    wait_for_a_key(-1, renderer);
    exitflag = 1;
  }

  /* printf("Loaded %d levels '%s'\n", levelscount, levcomment); */

  LevelSelectMenu:
  settings.tilesize = auto_tilesize(sprites);
  if (exitflag == 0) exitflag = flush_events();

  if (exitflag == 0) {
    curlevel = selectlevel(gameslist, sprites, renderer, window, &settings, levcomment, levelscount, curlevel, &levelfile);
    if (curlevel == SELECTLEVEL_BACK) {
      if (levelfile == NULL) {
        if (levelsource == LEVEL_INTERNET) goto LoadInternetLevels;
        goto GametypeSelectMenu;
      } else {
        exitflag = 1;
      }
    } else if (curlevel == SELECTLEVEL_QUIT) {
      exitflag = 1;
    } else if (curlevel == SELECTLEVEL_LOADFILE) {
      goto GametypeSelectMenu;
    }
  }
  if (exitflag == 0) fade2texture(renderer, window, sprites->black);
  if (exitflag == 0) loadlevel(&game, gameslist[curlevel], states);

  /* here we start the actual game */

  settings.tilesize = auto_tilesize(sprites);
  if ((curlevel == 0) && (game.solution == NULL)) showhelp = 1;
  playsolution = 0;
  drawscreenflags = 0;
  if (exitflag == 0) lastlevelleft = islevelthelastleft(gameslist, curlevel, levelscount);

  while (exitflag == 0) {
    if (playsolution > 0) {
      drawscreenflags |= DRAWSCREEN_PLAYBACK;
    } else {
      drawscreenflags &= ~DRAWSCREEN_PLAYBACK;
    }
    draw_screen(&game, states, sprites, renderer, window, &settings, 0, 0, 0, DRAWSCREEN_REFRESH | drawscreenflags, levcomment);
    if (showhelp != 0) {
      exitflag = displaytexture(renderer, sprites->help, window, -1, DISPLAYCENTERED, 255);
      draw_screen(&game, states, sprites, renderer, window, &settings, 0, 0, 0, DRAWSCREEN_REFRESH | drawscreenflags, levcomment);
      showhelp = 0;
    }
    if (debugmode != 0) printf("history: %s\n", states->history);

    /* Wait for an event - but ignore 'KEYUP' and 'MOUSEMOTION' events, since they are worthless in this game */
    for (;;) {
      if (SDL_WaitEventTimeout(&event, 80) == 0) {
        if (playsolution == 0) continue;
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_F10;
      }
      if ((event.type != SDL_KEYUP) && (event.type != SDL_MOUSEMOTION)) break;
    }

    /* check what event we got */
    if (event.type == SDL_QUIT) {
      exitflag = 1;
    } else if (event.type == SDL_DROPFILE) {
      if (processDropFileEvent(&event, &levelfile) != NULL) {
        fade2texture(renderer, window, sprites->black);
        goto GametypeSelectMenu;
      }
    } else if (event.type == SDL_KEYDOWN) {
      int res = 0;
      enum SOKMOVE movedir = sokmoveNONE;
      switch (normalizekeys(event.key.keysym.sym)) {
        case KEY_LEFT:
          movedir = sokmoveLEFT;
          break;
        case KEY_RIGHT:
          movedir = sokmoveRIGHT;
          break;
        case KEY_UP:
          movedir = sokmoveUP;
          break;
        case KEY_CTRL_UP:
          if (settings.tilesize < 255) settings.tilesize += 2;
          break;
        case KEY_DOWN:
          movedir = sokmoveDOWN;
          break;
        case KEY_CTRL_DOWN:
          if (settings.tilesize > 4) settings.tilesize -= 2;
          break;
        case KEY_BACKSPACE:
          if (playsolution == 0) sok_undo(&game, states);
          break;
        case KEY_R:
          playsolution = 0;
          loadlevel(&game, gameslist[curlevel], states);
          break;
        case KEY_F3: /* dump level & solution (if any) to clipboard */
          dumplevel2clipboard(gameslist[curlevel], gameslist[curlevel]->solution);
          exitflag = displaytexture(renderer, sprites->copiedtoclipboard, window, 2, DISPLAYCENTERED, 255);
          break;
        case KEY_CTRL_C:
          dumplevel2clipboard(&game, states->history);
          exitflag = displaytexture(renderer, sprites->snapshottoclipboard, window, 2, DISPLAYCENTERED, 255);
          break;
        case KEY_CTRL_V:
          {
          char *solFromClipboard;
          solFromClipboard = SDL_GetClipboardText();
          trimstr(solFromClipboard);
          if (isLegalSokoSolution(solFromClipboard) != 0) {
            loadlevel(&game, gameslist[curlevel], states);
            exitflag = displaytexture(renderer, sprites->playfromclipboard, window, 2, DISPLAYCENTERED, 255);
            playsolution = 1;
            if (playsource != NULL) free(playsource);
            playsource = unRLE(solFromClipboard);
            free(solFromClipboard);
          } else {
            if (solFromClipboard != NULL) free(solFromClipboard);
          }
          }
          break;
        case KEY_S:
          if (playsolution == 0) {
            if (game.solution != NULL) { /* only allow if there actually is a solution */
              if (playsource != NULL) free(playsource);
              playsource = unRLE(game.solution); /* I duplicate the solution string, because I want to free it later, since it can originate both from the game's solution as well as from a clipboard string */
              if (playsource != NULL) {
                loadlevel(&game, gameslist[curlevel], states);
                playsolution = 1;
              }
            } else {
              exitflag = displaytexture(renderer, sprites->nosolution, window, 1, DISPLAYCENTERED, 255);
            }
          }
          break;
        case KEY_F1:
          if (playsolution == 0) showhelp = 1;
          break;
        case KEY_F2:
          if ((drawscreenflags & DRAWSCREEN_NOBG) && (drawscreenflags & DRAWSCREEN_NOTXT)) {
            drawscreenflags &= ~(DRAWSCREEN_NOBG | DRAWSCREEN_NOTXT);
          } else if (drawscreenflags & DRAWSCREEN_NOBG) {
            drawscreenflags |= DRAWSCREEN_NOTXT;
          } else if (drawscreenflags & DRAWSCREEN_NOTXT) {
            drawscreenflags &= ~DRAWSCREEN_NOTXT;
            drawscreenflags |= DRAWSCREEN_NOBG;
          } else {
            drawscreenflags |= DRAWSCREEN_NOTXT;
          }
          break;
        case KEY_F5:
          if (playsolution == 0) {
            exitflag = displaytexture(renderer, sprites->saved, window, 1, DISPLAYCENTERED, 255);
            solution_save(game.crc32, states->history, "sav");
          }
          break;
        case KEY_F7:
          {
          char *loadsol;
          loadsol = solution_load(game.crc32, "sav");
          if (loadsol == NULL) {
            exitflag = displaytexture(renderer, sprites->nosave, window, 1, DISPLAYCENTERED, 255);
          } else {
            exitflag = displaytexture(renderer, sprites->loaded, window, 1, DISPLAYCENTERED, 255);
            playsolution = 0;
            loadlevel(&game, gameslist[curlevel], states);
            sok_play(&game, states, loadsol);
            free(loadsol);
          }
          }
          break;
        case KEY_FULLSCREEN:
          switchfullscreen(window);
          break;
        case KEY_ESCAPE:
          fade2texture(renderer, window, sprites->black);
          goto LevelSelectMenu;
      }
      if (playsolution > 0) {
        movedir = sokmoveNONE;
        switch (playsource[playsolution - 1]) {
          case 'u':
          case 'U':
            movedir = sokmoveUP;
            break;
          case 'r':
          case 'R':
            movedir = sokmoveRIGHT;
            break;
          case 'd':
          case 'D':
            movedir = sokmoveDOWN;
            break;
          case 'l':
          case 'L':
            movedir = sokmoveLEFT;
            break;
        }
        playsolution += 1;
        if (playsource[playsolution - 1] == 0) playsolution = 0;
      }
      if (movedir != sokmoveNONE) {
        if (sprites->playerid == SPRITE_PLAYERROTATE) rotatePlayer(sprites, &game, states, movedir, renderer, window, &settings, levcomment, drawscreenflags);
        res = sok_move(&game, movedir, 1, states);
        if (res >= 0) { /* do animations */
          int offset, offsetx = 0, offsety = 0, scrolling;
          int refreshnow = 1;
          if (res & sokmove_pushed) drawscreenflags |= DRAWSCREEN_PUSH;
          /* How will I need to move? */
          if (movedir == sokmoveUP) offsety = -1;
          if (movedir == sokmoveRIGHT) offsetx = 1;
          if (movedir == sokmoveDOWN) offsety = 1;
          if (movedir == sokmoveLEFT) offsetx = -1;
          sokDelay(0 - settings.framefreq);  /* init my delay timer */
          /* Will I need to move the player, or the entire field? */
          for (offset = 0; offset != settings.tilesize * offsetx; offset += offsetx) {
            if (refreshnow) {
              scrolling = scrollneeded(&game, window, settings.tilesize, offsetx, offsety);
              draw_screen(&game, states, sprites, renderer, window, &settings, offset, 0, scrolling, DRAWSCREEN_REFRESH | drawscreenflags, levcomment);
            }
            refreshnow = sokDelay((settings.framedelay * 12) / settings.tilesize); /* wait a moment and check if it's time to refresh */
          }
          for (offset = 0; offset != settings.tilesize * offsety; offset += offsety) {
            if (refreshnow) {
              scrolling = scrollneeded(&game, window, settings.tilesize, offsetx, offsety);
              draw_screen(&game, states, sprites, renderer, window, &settings, 0, offset, scrolling, DRAWSCREEN_REFRESH | drawscreenflags, levcomment);
            }
            refreshnow = sokDelay((settings.framedelay * 12) / settings.tilesize); /* wait a moment and check if it's time to refresh */
          }
        }
        res = sok_move(&game, movedir, 0, states);
        if ((res >= 0) && (res & sokmove_solved)) {
          unsigned short alphaval;
          SDL_Texture *tmptex;
          /* display a congrats message */
          if (lastlevelleft != 0) {
            tmptex = sprites->congrats;
          } else {
            tmptex = sprites->cleared;
          }
          flush_events();
          for (alphaval = 0; alphaval < 255; alphaval += 30) {
            draw_screen(&game, states, sprites, renderer, window, &settings, 0, 0, 0, drawscreenflags, levcomment);
            exitflag = displaytexture(renderer, tmptex, window, 0, DISPLAYCENTERED, (unsigned char)alphaval);
            SDL_Delay(25);
            if (exitflag != 0) break;
          }
          if (exitflag == 0) {
            draw_screen(&game, states, sprites, renderer, window, &settings, 0, 0, 0, drawscreenflags, levcomment);
            /* if this was the last level left, display a congrats screen */
            if (lastlevelleft != 0) {
              exitflag = displaytexture(renderer, sprites->congrats, window, 10, DISPLAYCENTERED, 255);
            } else {
              exitflag = displaytexture(renderer, sprites->cleared, window, 3, DISPLAYCENTERED, 255);
            }
            /* fade out to black */
            if (exitflag == 0) {
              fade2texture(renderer, window, sprites->black);
              exitflag = flush_events();
            }
          }
          /* load the new level and reset states */
          curlevel = -1; /* this will make the selectlevel() function to preselect the next level automatically */
          goto LevelSelectMenu;
        }
      }
      drawscreenflags &= ~DRAWSCREEN_PUSH;
    }

    if (exitflag != 0) break;
  }

  /* free the states struct */
  sok_freestates(states);

  if (levelfile != NULL) free(levelfile);

  /* free all textures */
  skin_free(sprites);

  /* clean up SDL */
  flush_events();
  SDL_DestroyWindow(window);
  SDL_Quit();

  /* Clean-up networking. */
  cleanup_net();

  return(0);
}
