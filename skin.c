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

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

#include "gra.h"
#include "gz.h"

#include "skin.h"


/* loads a bmp.gz graphic and returns it as a texture, NULL on error */
static SDL_Texture *loadGraphic(SDL_Renderer *renderer, const void *memptr, size_t memlen) {
  SDL_Surface *surface;
  SDL_Texture *texture;

  surface = loadgzbmp(memptr, memlen);
  if (surface == NULL) {
    puts("loadgzbmp() failed!");
    return(NULL);
  }
  texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  if (texture == NULL) {
    printf("SDL_CreateTextureFromSurface() failed: %s\n", SDL_GetError());
    return(NULL);
  }

  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  return(texture);
}


/* looks out for a skin file and opens it, if found */
static FILE *skin_lookup(const char *name) {
  FILE *fd = NULL;
  struct skinlist *list, *node;

  /* load list of available skins and look for a match with name */
  list = skin_list();
  for (node = list; node != NULL; node = node->next) {
    if (strcmp(node->name, name) == 0) break;
  }

  /* if found a match, open it */
  if (node != NULL) {
    fd = fopen(node->path, "rb");
    printf("found skin file at %s\n", node->path);
    if (fd == NULL) fprintf(stderr, "failed to open skin file: %s\n", strerror(errno));
  }

  /* free skin list */
  skin_list_free(list);

  return(fd);
}


void skin_list_free(struct skinlist *l) {
  struct skinlist *next;
  while (l != NULL) {
    next = l->next;
    free(l->name);
    free(l->path);
    free(l);
    l = next;
  }
}


/* filter out skin names to match bmp.gz files only */
static int skin_filter(const struct dirent *d) {
  char *ext = strstr(d->d_name, ".bmp.gz");
  if ((ext == NULL) || (strlen(ext) != 7)) return(0);
  return(1);
}


struct skinlist *skin_list(void) {
  char *dirs[4];
  int i;
  struct skinlist *r = NULL;
  char *ptr;
  char buff[512];

  /* directories where I should look for skins */
  i = 0;
  /* applications running path */
  ptr = SDL_GetBasePath();
  if (ptr != NULL) {
    snprintf(buff, sizeof(buff), "%sskins/", ptr);
    dirs[i++] = strdup(buff);
    SDL_free(ptr);
  }
  /* /usr/share/... */
  dirs[i++] = strdup("/usr/share/simplesok/skins/");
  /* local user preferences directory */
  ptr = SDL_GetPrefPath("", "simplesok");
  if (ptr != NULL) {
    snprintf(buff, sizeof(buff), "%sskins/", ptr);
    dirs[i++] = strdup(buff);
    SDL_free(ptr);
  }
  /* append an end-of-list marker */
  dirs[i] = NULL;

  /* look in each directory (and free the dir string) */
  for (i = 0; dirs[i] != NULL; i++) {
    DIR *dirfd;
    struct dirent *dentry;

    dirfd = opendir(dirs[i]);
    if (dirfd == NULL) continue;

    while ((dentry = readdir(dirfd)) != NULL) {
      char fname[512];
      struct skinlist *node;

      if (skin_filter(dentry) == 0) continue;

      /* prep new node */
      snprintf(fname, sizeof(fname), "%s%s", dirs[i], dentry->d_name);
      node = calloc(1, sizeof(struct skinlist));
      node->name = strdup(dentry->d_name);
      node->path = strdup(fname);

      /* trim file extension from skin name */
      {
        size_t len = strlen(node->name);
        if (len > 7) node->name[len - 7] = 0;
      }

      /* append new node to list of results */
      node->next = r;
      r = node;

    }
    closedir(dirfd);
  }

  /* free dirs entries */
  for (i = 0; dirs[i] != NULL; i++) free(dirs[i]);

  return(r);
}


struct spritesstruct *skin_load(const char *name, SDL_Renderer *renderer) {
  struct spritesstruct *sprites;
  int i;
  FILE *fd;
  int gwidth;

  sprites = calloc(sizeof(struct spritesstruct), 1);
  if (sprites == NULL) {
    printf("skin.c: out of memory @ %d\n", __LINE__);
    return(NULL);
  }

  /* look out for a skin file */
  if ((name != NULL) && ((fd = skin_lookup(name)) != NULL)) {
    unsigned char *memptr;
    size_t skinlen;
    memptr = malloc(1024 * 1024);
    skinlen = fread(memptr, 1, 1024 * 1024, fd);
    fclose(fd);
    sprites->map = loadGraphic(renderer, memptr, skinlen);
    free(memptr);
  } else { /* otherwise load the embedded skin */
    fprintf(stderr, "skin load failed ('%s'), falling back to embedded default\n", name);
    sprites->map = loadGraphic(renderer, skins_yoshi_bmp_gz, skins_yoshi_bmp_gz_len);
  }
  /* a sprite map is 8 tiles wide with each tile having a 1px margin */
  SDL_QueryTexture(sprites->map, NULL, NULL, &gwidth, NULL);
  sprites->tilesize = (unsigned short)(gwidth - 9) / 8; /* 8 tiles, 9 pixels of total margins per line */

  /* playfield items */
  sprites->black = loadGraphic(renderer, assets_img_black_bmp_gz, assets_img_black_bmp_gz_len);

  /* strings */
  sprites->cleared = loadGraphic(renderer, assets_img_cleared_bmp_gz, assets_img_cleared_bmp_gz_len);
  sprites->help = loadGraphic(renderer, assets_img_help_bmp_gz, assets_img_help_bmp_gz_len);
  sprites->solved = loadGraphic(renderer, assets_img_solved_bmp_gz, assets_img_solved_bmp_gz_len);
  sprites->nosolution = loadGraphic(renderer, assets_img_nosol_bmp_gz, assets_img_nosol_bmp_gz_len);
  sprites->congrats = loadGraphic(renderer, assets_img_congrats_bmp_gz, assets_img_congrats_bmp_gz_len);
  sprites->copiedtoclipboard = loadGraphic(renderer, assets_img_copiedtoclipboard_bmp_gz, assets_img_copiedtoclipboard_bmp_gz_len);
  sprites->playfromclipboard = loadGraphic(renderer, assets_img_playfromclipboard_bmp_gz, assets_img_playfromclipboard_bmp_gz_len);
  sprites->snapshottoclipboard = loadGraphic(renderer, assets_img_snapshottoclipboard_bmp_gz, assets_img_snapshottoclipboard_bmp_gz_len);
  sprites->saved = loadGraphic(renderer, assets_img_saved_bmp_gz, assets_img_saved_bmp_gz_len);
  sprites->loaded = loadGraphic(renderer, assets_img_loaded_bmp_gz, assets_img_loaded_bmp_gz_len);
  sprites->nosave = loadGraphic(renderer, assets_img_nosave_bmp_gz, assets_img_nosave_bmp_gz_len);

  /* load font */
  sprites->font['0'] = loadGraphic(renderer, assets_font_0_bmp_gz, assets_font_0_bmp_gz_len);
  sprites->font['1'] = loadGraphic(renderer, assets_font_1_bmp_gz, assets_font_1_bmp_gz_len);
  sprites->font['2'] = loadGraphic(renderer, assets_font_2_bmp_gz, assets_font_2_bmp_gz_len);
  sprites->font['3'] = loadGraphic(renderer, assets_font_3_bmp_gz, assets_font_3_bmp_gz_len);
  sprites->font['4'] = loadGraphic(renderer, assets_font_4_bmp_gz, assets_font_4_bmp_gz_len);
  sprites->font['5'] = loadGraphic(renderer, assets_font_5_bmp_gz, assets_font_5_bmp_gz_len);
  sprites->font['6'] = loadGraphic(renderer, assets_font_6_bmp_gz, assets_font_6_bmp_gz_len);
  sprites->font['7'] = loadGraphic(renderer, assets_font_7_bmp_gz, assets_font_7_bmp_gz_len);
  sprites->font['8'] = loadGraphic(renderer, assets_font_8_bmp_gz, assets_font_8_bmp_gz_len);
  sprites->font['9'] = loadGraphic(renderer, assets_font_9_bmp_gz, assets_font_9_bmp_gz_len);
  sprites->font['a'] = loadGraphic(renderer, assets_font_a_bmp_gz, assets_font_a_bmp_gz_len);
  sprites->font['b'] = loadGraphic(renderer, assets_font_b_bmp_gz, assets_font_b_bmp_gz_len);
  sprites->font['c'] = loadGraphic(renderer, assets_font_c_bmp_gz, assets_font_c_bmp_gz_len);
  sprites->font['d'] = loadGraphic(renderer, assets_font_d_bmp_gz, assets_font_d_bmp_gz_len);
  sprites->font['e'] = loadGraphic(renderer, assets_font_e_bmp_gz, assets_font_e_bmp_gz_len);
  sprites->font['f'] = loadGraphic(renderer, assets_font_f_bmp_gz, assets_font_f_bmp_gz_len);
  sprites->font['g'] = loadGraphic(renderer, assets_font_g_bmp_gz, assets_font_g_bmp_gz_len);
  sprites->font['h'] = loadGraphic(renderer, assets_font_h_bmp_gz, assets_font_h_bmp_gz_len);
  sprites->font['i'] = loadGraphic(renderer, assets_font_i_bmp_gz, assets_font_i_bmp_gz_len);
  sprites->font['j'] = loadGraphic(renderer, assets_font_j_bmp_gz, assets_font_j_bmp_gz_len);
  sprites->font['k'] = loadGraphic(renderer, assets_font_k_bmp_gz, assets_font_k_bmp_gz_len);
  sprites->font['l'] = loadGraphic(renderer, assets_font_l_bmp_gz, assets_font_l_bmp_gz_len);
  sprites->font['m'] = loadGraphic(renderer, assets_font_m_bmp_gz, assets_font_m_bmp_gz_len);
  sprites->font['n'] = loadGraphic(renderer, assets_font_n_bmp_gz, assets_font_n_bmp_gz_len);
  sprites->font['o'] = loadGraphic(renderer, assets_font_o_bmp_gz, assets_font_o_bmp_gz_len);
  sprites->font['p'] = loadGraphic(renderer, assets_font_p_bmp_gz, assets_font_p_bmp_gz_len);
  sprites->font['q'] = loadGraphic(renderer, assets_font_q_bmp_gz, assets_font_q_bmp_gz_len);
  sprites->font['r'] = loadGraphic(renderer, assets_font_r_bmp_gz, assets_font_r_bmp_gz_len);
  sprites->font['s'] = loadGraphic(renderer, assets_font_s_bmp_gz, assets_font_s_bmp_gz_len);
  sprites->font['t'] = loadGraphic(renderer, assets_font_t_bmp_gz, assets_font_t_bmp_gz_len);
  sprites->font['u'] = loadGraphic(renderer, assets_font_u_bmp_gz, assets_font_u_bmp_gz_len);
  sprites->font['v'] = loadGraphic(renderer, assets_font_v_bmp_gz, assets_font_v_bmp_gz_len);
  sprites->font['w'] = loadGraphic(renderer, assets_font_w_bmp_gz, assets_font_w_bmp_gz_len);
  sprites->font['x'] = loadGraphic(renderer, assets_font_x_bmp_gz, assets_font_x_bmp_gz_len);
  sprites->font['y'] = loadGraphic(renderer, assets_font_y_bmp_gz, assets_font_y_bmp_gz_len);
  sprites->font['z'] = loadGraphic(renderer, assets_font_z_bmp_gz, assets_font_z_bmp_gz_len);
  sprites->font['A'] = loadGraphic(renderer, assets_font_aa_bmp_gz, assets_font_aa_bmp_gz_len);
  sprites->font['B'] = loadGraphic(renderer, assets_font_bb_bmp_gz, assets_font_bb_bmp_gz_len);
  sprites->font['C'] = loadGraphic(renderer, assets_font_cc_bmp_gz, assets_font_cc_bmp_gz_len);
  sprites->font['D'] = loadGraphic(renderer, assets_font_dd_bmp_gz, assets_font_dd_bmp_gz_len);
  sprites->font['E'] = loadGraphic(renderer, assets_font_ee_bmp_gz, assets_font_ee_bmp_gz_len);
  sprites->font['F'] = loadGraphic(renderer, assets_font_ff_bmp_gz, assets_font_ff_bmp_gz_len);
  sprites->font['G'] = loadGraphic(renderer, assets_font_gg_bmp_gz, assets_font_gg_bmp_gz_len);
  sprites->font['H'] = loadGraphic(renderer, assets_font_hh_bmp_gz, assets_font_hh_bmp_gz_len);
  sprites->font['I'] = loadGraphic(renderer, assets_font_ii_bmp_gz, assets_font_ii_bmp_gz_len);
  sprites->font['J'] = loadGraphic(renderer, assets_font_jj_bmp_gz, assets_font_jj_bmp_gz_len);
  sprites->font['K'] = loadGraphic(renderer, assets_font_kk_bmp_gz, assets_font_kk_bmp_gz_len);
  sprites->font['L'] = loadGraphic(renderer, assets_font_ll_bmp_gz, assets_font_ll_bmp_gz_len);
  sprites->font['M'] = loadGraphic(renderer, assets_font_mm_bmp_gz, assets_font_mm_bmp_gz_len);
  sprites->font['N'] = loadGraphic(renderer, assets_font_nn_bmp_gz, assets_font_nn_bmp_gz_len);
  sprites->font['O'] = loadGraphic(renderer, assets_font_oo_bmp_gz, assets_font_oo_bmp_gz_len);
  sprites->font['P'] = loadGraphic(renderer, assets_font_pp_bmp_gz, assets_font_pp_bmp_gz_len);
  sprites->font['Q'] = loadGraphic(renderer, assets_font_qq_bmp_gz, assets_font_qq_bmp_gz_len);
  sprites->font['R'] = loadGraphic(renderer, assets_font_rr_bmp_gz, assets_font_rr_bmp_gz_len);
  sprites->font['S'] = loadGraphic(renderer, assets_font_ss_bmp_gz, assets_font_ss_bmp_gz_len);
  sprites->font['T'] = loadGraphic(renderer, assets_font_tt_bmp_gz, assets_font_tt_bmp_gz_len);
  sprites->font['U'] = loadGraphic(renderer, assets_font_uu_bmp_gz, assets_font_uu_bmp_gz_len);
  sprites->font['V'] = loadGraphic(renderer, assets_font_vv_bmp_gz, assets_font_vv_bmp_gz_len);
  sprites->font['W'] = loadGraphic(renderer, assets_font_ww_bmp_gz, assets_font_ww_bmp_gz_len);
  sprites->font['X'] = loadGraphic(renderer, assets_font_xx_bmp_gz, assets_font_xx_bmp_gz_len);
  sprites->font['Y'] = loadGraphic(renderer, assets_font_yy_bmp_gz, assets_font_yy_bmp_gz_len);
  sprites->font['Z'] = loadGraphic(renderer, assets_font_zz_bmp_gz, assets_font_zz_bmp_gz_len);
  sprites->font[':'] = loadGraphic(renderer, assets_font_sym_col_bmp_gz, assets_font_sym_col_bmp_gz_len);
  sprites->font[';'] = loadGraphic(renderer, assets_font_sym_scol_bmp_gz, assets_font_sym_scol_bmp_gz_len);
  sprites->font['!'] = loadGraphic(renderer, assets_font_sym_excl_bmp_gz, assets_font_sym_excl_bmp_gz_len);
  sprites->font['$'] = loadGraphic(renderer, assets_font_sym_doll_bmp_gz, assets_font_sym_doll_bmp_gz_len);
  sprites->font['.'] = loadGraphic(renderer, assets_font_sym_dot_bmp_gz, assets_font_sym_dot_bmp_gz_len);
  sprites->font['&'] = loadGraphic(renderer, assets_font_sym_ampe_bmp_gz, assets_font_sym_ampe_bmp_gz_len);
  sprites->font['*'] = loadGraphic(renderer, assets_font_sym_star_bmp_gz, assets_font_sym_star_bmp_gz_len);
  sprites->font[','] = loadGraphic(renderer, assets_font_sym_comm_bmp_gz, assets_font_sym_comm_bmp_gz_len);
  sprites->font['('] = loadGraphic(renderer, assets_font_sym_par1_bmp_gz, assets_font_sym_par1_bmp_gz_len);
  sprites->font[')'] = loadGraphic(renderer, assets_font_sym_par2_bmp_gz, assets_font_sym_par2_bmp_gz_len);
  sprites->font['['] = loadGraphic(renderer, assets_font_sym_bra1_bmp_gz, assets_font_sym_bra1_bmp_gz_len);
  sprites->font[']'] = loadGraphic(renderer, assets_font_sym_bra2_bmp_gz, assets_font_sym_bra2_bmp_gz_len);
  sprites->font['-'] = loadGraphic(renderer, assets_font_sym_minu_bmp_gz, assets_font_sym_minu_bmp_gz_len);
  sprites->font['_'] = loadGraphic(renderer, assets_font_sym_unde_bmp_gz, assets_font_sym_unde_bmp_gz_len);
  sprites->font['/'] = loadGraphic(renderer, assets_font_sym_slas_bmp_gz, assets_font_sym_slas_bmp_gz_len);
  sprites->font['"'] = loadGraphic(renderer, assets_font_sym_quot_bmp_gz, assets_font_sym_quot_bmp_gz_len);
  sprites->font['#'] = loadGraphic(renderer, assets_font_sym_hash_bmp_gz, assets_font_sym_hash_bmp_gz_len);
  sprites->font['@'] = loadGraphic(renderer, assets_font_sym_at_bmp_gz, assets_font_sym_at_bmp_gz_len);
  sprites->font['\''] = loadGraphic(renderer, assets_font_sym_apos_bmp_gz, assets_font_sym_apos_bmp_gz_len);

  /* set all NULL fonts to '_' */
  for (i = 0; i < 256; i++) {
    if (sprites->font[i] == NULL) sprites->font[i] = sprites->font['_'];
  }

  /* analyze the PLAYERROTATE position - if completely transparent, then player character is static */
  sprites->playerid = SPRITE_PLAYERSTATIC;
  {
    SDL_Rect r;
    uint32_t *pixels;

    pixels = malloc(sprites->tilesize * sprites->tilesize * 4);

    r.x = 0;
    r.y = 0;
    r.w = sprites->tilesize;
    r.h = sprites->tilesize;

    /* clear the entire screen to bright pink so I can easily determine if player's rotate sprite is empty or not */
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); /* restore current color to black */

    gra_rendertile(renderer, sprites, SPRITE_PLAYERROTATE, 0, 0, sprites->tilesize, 0);
    if (SDL_RenderReadPixels(renderer, &r, SDL_PIXELFORMAT_RGBA8888, pixels, sprites->tilesize * 4) != 0) {
      printf("OOOPS: %s\n", SDL_GetError());
    }
    for (i = 0; i < sprites->tilesize * sprites->tilesize; i++) {
      if (pixels[i] != 0xff00ffff) {
        sprites->playerid = SPRITE_PLAYERROTATE;
        break;
      }
    }
    free(pixels);
  }

  /* compute the em unit used to scale other things in the game */
  {
    int em;
    /* the reference is the height of the 'A' glyph */
    SDL_QueryTexture(sprites->font['A'], NULL, NULL, NULL, &em);
    sprites->em = (unsigned short)em;
  }

  return(sprites);
}


void skin_free(struct spritesstruct *sprites) {
  int x;
  if (sprites->map) SDL_DestroyTexture(sprites->map);
  if (sprites->black) SDL_DestroyTexture(sprites->black);
  if (sprites->nosolution) SDL_DestroyTexture(sprites->nosolution);
  if (sprites->cleared) SDL_DestroyTexture(sprites->cleared);
  if (sprites->help) SDL_DestroyTexture(sprites->help);
  if (sprites->congrats) SDL_DestroyTexture(sprites->congrats);
  if (sprites->copiedtoclipboard) SDL_DestroyTexture(sprites->copiedtoclipboard);
  if (sprites->playfromclipboard) SDL_DestroyTexture(sprites->playfromclipboard);
  if (sprites->snapshottoclipboard) SDL_DestroyTexture(sprites->snapshottoclipboard);
  if (sprites->saved) SDL_DestroyTexture(sprites->saved);
  if (sprites->loaded) SDL_DestroyTexture(sprites->loaded);
  if (sprites->nosave) SDL_DestroyTexture(sprites->nosave);
  for (x = 0; x < 256; x++) {
    /* skip font['_'] because it may be used as a placeholder pointer for many
     * glyphs, so I will explicitely free it afterwards */
    if (sprites->font[x] && sprites->font[x] != sprites->font['_']) {
      SDL_DestroyTexture(sprites->font[x]);
    }
  }
  if (sprites->font['_']) SDL_DestroyTexture(sprites->font['_']);
  free(sprites);
}
