/*
 * wrappers and helper functions around graphic operations.
 *
 * Copyright (C) 2014-2023 Mateusz Viste
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>

#include <SDL2/SDL.h>

#include "gra.h"
#include "gz.h"
#include "skin.h"


/* loads a gziped bmp image from memory and returns a surface */
SDL_Surface *loadgzbmp(const unsigned char *memgz, size_t memgzlen) {
  SDL_RWops *rwop;
  SDL_Surface *surface;
  unsigned char *rawimage;
  size_t rawimagelen;
  if (isGz(memgz, memgzlen) == 0) return(NULL);
  rawimage = ungz(memgz, memgzlen, &rawimagelen);
  rwop = SDL_RWFromMem(rawimage, (int)rawimagelen);
  surface = SDL_LoadBMP_RW(rwop, 0);
  SDL_FreeRW(rwop);
  free(rawimage);
  return(surface);
}


/* fills rect with coordinates of tile id withing sprite map */
static void locate_sprite(SDL_Rect *r, unsigned short id, const struct spritesstruct *spr) {
  r->x = 1 + (id % 8) * (spr->tilesize + 1);
  r->y = 1 + (id / 8) * (spr->tilesize + 1);
  r->w = spr->tilesize;
  r->h = spr->tilesize;
}


/* render a tiled background over the entire screen */
void gra_renderbg(SDL_Renderer *renderer, struct spritesstruct *spr, unsigned short id, unsigned short tilesize, int winw, int winh) {
  SDL_Rect src, dst;

  /* prep src rect from sprite map */
  locate_sprite(&src, id, spr);
  src.w *= 2;
  src.h *= 2;

  dst.w = tilesize * 2;
  dst.h = tilesize * 2;

  /* fill screen with tiles */
  for (dst.y = 0; dst.y < winh; dst.y += dst.h) {
    for (dst.x = 0; dst.x < winw; dst.x += dst.w) {
      SDL_RenderCopy(renderer, spr->map, &src, &dst);
    }
  }
}


void gra_rendertile(SDL_Renderer *renderer, struct spritesstruct *spr, unsigned short id, int x, int y, unsigned short tilesize, int angle) {
  SDL_Rect src, dst;

  /* prep src rect from sprite map */
  locate_sprite(&src, id, spr);

  /* prep dst */
  dst.x = x;
  dst.y = y;
  dst.w = tilesize;
  dst.h = tilesize;

  if ((angle == 0) || (id == SPRITE_PLAYERSTATIC)) {
    SDL_RenderCopy(renderer, spr->map, &src, &dst);
  } else {
    SDL_RenderCopyEx(renderer, spr->map, &src, &dst, angle, NULL, SDL_FLIP_NONE);
  }
}
