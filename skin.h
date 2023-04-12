/*
 * loading skins
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

#ifndef SKIN_H
#define SKIN_H

#include <SDL2/SDL.h>

#include "gra.h"

#define SPRITE_BOX    0
#define SPRITE_BOXOK  1
#define SPRITE_GOAL   2
#define SPRITE_FLOOR  3
#define SPRITE_PLAYERROTATE 4
#define SPRITE_PLAYERSTATIC 5
#define SPRITE_WALLCR 8  /* plain wall corners */
#define SPRITE_WALL0  16
#define SPRITE_BG     6

struct skinlist {
  struct skinlist *next;
  char *name;
  char *path;
};

struct skinlist *skin_list(void);
void skin_list_free(struct skinlist *l);

struct spritesstruct *skin_load(const char *name, SDL_Renderer *renderer);
void skin_free(struct spritesstruct *skin);

#endif
