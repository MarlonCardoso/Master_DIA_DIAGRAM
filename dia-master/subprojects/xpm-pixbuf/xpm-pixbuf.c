/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* GdkPixbuf library - XPM image loader
 *
 * Copyright (C) 1999 Mark Crichton
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Mark Crichton <crichton@gimp.org>
 *          Federico Mena-Quintero <federico@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "xpm-colour-table.h"
#include "xpm-pixbuf.h"


typedef struct {
  XpmColour colour;
  gboolean transparent;
} XpmResolvedColour;


static inline gboolean
parse_colour (const char *const restrict spec,
              XpmColour  *const restrict colour)
{
  unsigned int red, green, blue;
  int i;

  if (G_UNLIKELY (spec[0] != '#')) {
    return xpm_colour_table_loopup (spec, colour);
  }

  if ((i = strlen (spec + 1)) % 3) {
    return FALSE;
  }
  i /= 3;

  if (i == 4) {
    if (sscanf (spec + 1, "%4x%4x%4x", &red, &green, &blue) != 3) {
      return FALSE;
    }
    colour->red = red;
    colour->green = green;
    colour->blue = blue;
  } else if (i == 1) {
    if (sscanf (spec + 1, "%1x%1x%1x", &red, &green, &blue) != 3) {
      return FALSE;
    }
    colour->red = (red * 65535) / 15;
    colour->green = (green * 65535) / 15;
    colour->blue = (blue * 65535) / 15;
  } else if (i == 2) {
    if (sscanf (spec + 1, "%2x%2x%2x", &red, &green, &blue) != 3) {
      return FALSE;
    }
    colour->red = (red * 65535) / 255;
    colour->green = (green * 65535) / 255;
    colour->blue = (blue * 65535) / 255;
  } else /* if (i == 3) */ {
    if (sscanf (spec + 1, "%3x%3x%3x", &red, &green, &blue) != 3) {
      return FALSE;
    }
    colour->red = (red * 65535) / 4095;
    colour->green = (green * 65535) / 4095;
    colour->blue = (blue * 65535) / 4095;
  }

  return TRUE;
}


static inline char *
extract_colour (const char *const restrict buffer)
{
  const char *p = &buffer[0];
  int new_key = 0;
  int key = 0;
  int current_key = 1;
  size_t space = 128;
  char word[129], colour[129], current_colour[129];
  char *r;

  word[0] = '\0';
  colour[0] = '\0';
  current_colour[0] = '\0';

  while (1) {
    /* skip whitespace */
    for (; *p != '\0' && g_ascii_isspace (*p); p++) { }

    /* copy word */
    for (r = word;
         *p != '\0' && !g_ascii_isspace (*p) && r - word < ((ssize_t) sizeof (word) - 1);
         p++, r++) {
      *r = *p;
    }
    *r = '\0';

    if (*word == '\0') {
      if (colour[0] == '\0') {
        /* incomplete colourmap entry */
        return NULL;
      } else {
        /* end of entry, still store the last colour */
        new_key = 1;
      }
    } else if (key > 0 && colour[0] == '\0') {
      /* next word must be a colour name part */
      new_key = 0;
    } else {
      if (strcmp (word, "c") == 0) {
        new_key = 5;
      } else if (strcmp (word, "g") == 0) {
        new_key = 4;
      } else if (strcmp (word, "g4") == 0) {
        new_key = 3;
      } else if (strcmp (word, "m") == 0) {
        new_key = 2;
      } else if (strcmp (word, "s") == 0) {
        new_key = 1;
      } else {
        new_key = 0;
      }
    }

    if (new_key == 0) {  /* word is a colour name part */
      if (key == 0) {
        /* key expected */
        return NULL;
      }
      /* accumulate colour name */
      if (colour[0] != '\0') {
        strncat (colour, " ", space);
        space -= MIN (space, 1);
      }
      strncat (colour, word, space);
      space -= MIN (space, strlen (word));
    } else {  /* word is a key */
      if (key > current_key) {
        current_key = key;
        strcpy (current_colour, colour);
      }
      space = 128;
      colour[0] = '\0';
      key = new_key;
      if (*p == '\0') {
        break;
      }
    }
  }

  if (current_key > 1) {
    /* colour names are case insensitive */
    return g_ascii_strdown (current_colour, -1);
  } else {
    return NULL;
  }
}


static inline const char *
next_line (const char *const restrict data[const restrict],
           size_t     *const restrict offset)
{
  const char *retval = data[*offset];

  if (!retval) {
    return NULL;
  }

  *offset += 1;

  return retval;
}


static inline gboolean
read_header (const char *const restrict data[const restrict],
             size_t     *const restrict offset,
             size_t     *const restrict width,
             size_t     *const restrict height,
             size_t     *const restrict n_colours,
             size_t     *const restrict tag_size)
{
  int w, h, n_col, cpp, items, x_hot, y_hot;
  const char *buffer = next_line (data, offset);

  if (G_UNLIKELY (!buffer)) {
    g_warning ("Broken Icon: No Header");
    return FALSE;
  }

  items = sscanf (buffer, "%d %d %d %d %d %d", &w, &h, &n_col, &cpp, &x_hot, &y_hot);

  if (G_UNLIKELY (items != 4 && items != 6)) {
    g_warning ("Broken Icon: Invalid Header");
    return FALSE;
  }

  if (G_UNLIKELY (w <= 0 || h <= 0)) {
    g_warning ("Broken Icon: Must be at least 1×1");
    return FALSE;
  }

  *width = w;
  *height = h;

  /* Check from libXpm's ParsePixels() */
  if ((h > 0 && (*width) >= G_MAXUINT / h) ||
      ((*width) * (*height)) >= G_MAXUINT / sizeof (unsigned int)) {
    g_warning ("Broken Icon: Invalid Header");
    return FALSE;
  }

  if (cpp <= 0 || cpp >= 32 || w >= G_MAXINT / cpp) {
    g_warning ("Broken Icon: Invalid tag size");
    return FALSE;
  }

  if (n_col <= 0 ||
      n_col >= G_MAXINT / (cpp + 1) ||
      (size_t) n_col >= G_MAXINT / sizeof (XpmResolvedColour)) {
    g_warning ("Broken Icon: Invalid colour count (%i)", n_col);
    return FALSE;
  }

  *n_colours = n_col;
  *tag_size = cpp;

  return TRUE;
}


static inline GHashTable *
read_colours (const char                  *const restrict data[const restrict],
              size_t                      *const restrict offset,
              const size_t                                n_colours,
              const size_t                                tag_size,
              char              *restrict *const restrict names,
              gboolean                    *const restrict is_trans,
              XpmResolvedColour *restrict *const restrict fallback)
{
  GHashTable *colour_map;
  char *tag_buffer;
  XpmResolvedColour *colours;
  XpmResolvedColour *colour;

  colour_map = g_hash_table_new (g_str_hash, g_str_equal);

  tag_buffer = g_try_malloc (n_colours * (tag_size + 1));
  if (!tag_buffer) {
    g_warning ("Icon too big for memory");
    goto failed;
  }

  colours = g_try_malloc (sizeof (XpmResolvedColour) * n_colours);
  if (!colours) {
    g_warning ("Icon too big for memory");
    goto failed;
  }

  for (size_t cnt = 0; cnt < n_colours; cnt++) {
    const char *buffer;
    char *colour_name;
    char *colour_string;

    buffer = next_line (data, offset);
    if (!buffer) {
      g_warning ("Broken Icon: Palette incomplete");
      goto failed;
    }

    colour = &colours[cnt];
    colour_string = &tag_buffer[cnt * (tag_size + 1)];
    strncpy (colour_string, buffer, tag_size);
    colour_string[tag_size] = 0;
    buffer += strlen (colour_string);
    colour->transparent = FALSE;

    colour_name = extract_colour (buffer);

    if ((colour_name == NULL)
        || (g_strcmp0 (colour_name, "none") == 0)
        || (parse_colour (colour_name, &colour->colour) == FALSE)) {
      colour->transparent = TRUE;
      colour->colour = (XpmColour) { 0, };
      *is_trans = TRUE;
    }

    g_clear_pointer (&colour_name, g_free);
    g_hash_table_insert (colour_map, colour_string, colour);

    if (G_UNLIKELY (cnt == 0)) {
      *fallback = colour;
    }
  }

  *names = tag_buffer;

  return colour_map;

failed:
  g_clear_pointer (&colour_map, g_hash_table_unref);
  g_clear_pointer (&tag_buffer, g_free);

  return NULL;
}


/**
 * xpm_pixbuf_load:
 * @data: the XPM data to render
 *
 * Returns: (transfer full): a [type@Gdk.Pixbuf] representing @data
 */
GdkPixbuf *
xpm_pixbuf_load (const char *const restrict data[const restrict])
{
  size_t width, height, n_colours, tag_size, wbytes, offset = 0;
  gboolean is_trans = FALSE;
  char *name_buf = NULL;
  GHashTable *colour_map = NULL;
  XpmResolvedColour *fallback = NULL;
  GdkPixbuf *pixbuf = NULL;
  int rowstride;

  if (G_UNLIKELY (!read_header (data, &offset, &width, &height, &n_colours, &tag_size))) {
    goto out;
  }

  colour_map = read_colours (data, &offset, n_colours, tag_size, &name_buf, &is_trans, &fallback);
  if (G_UNLIKELY (!colour_map)) {
    goto out;
  }

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, is_trans, 8, width, height);
  if (!pixbuf) {
    g_warning ("Icon too big for memory");
    goto out;
  }

  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  wbytes = width * tag_size;

  for (size_t ycnt = 0; ycnt < height; ycnt++) {
    guchar *pixtmp = gdk_pixbuf_get_pixels (pixbuf) + ycnt * rowstride;
    const char *buffer = next_line (data, &offset);

    if ((!buffer) || (strlen (buffer) < wbytes)) {
      /* Advertised width doesn't match pixels */
      g_warning ("Bad Icon: Expected row of %"G_GSIZE_FORMAT" bytes, got %"G_GSIZE_FORMAT,
                 wbytes,
                 strlen (buffer));
      goto out;
    }

    for (size_t n = 0, xcnt = 0; n < wbytes; n += tag_size, xcnt++) {
      XpmResolvedColour *colour;
      char pixel_str[32];

      strncpy (pixel_str, &buffer[n], tag_size);
      pixel_str[tag_size] = 0;

      colour = g_hash_table_lookup (colour_map, pixel_str);

      /* Bad XPM...punt */
      if (!colour) {
        colour = fallback;
      }

      *pixtmp++ = colour->colour.red >> 8;
      *pixtmp++ = colour->colour.green >> 8;
      *pixtmp++ = colour->colour.blue >> 8;

      if (is_trans && colour->transparent)
        *pixtmp++ = 0;
      else if (is_trans) {
        *pixtmp++ = 0xFF;
      }
    }
  }

  g_clear_pointer (&colour_map, g_hash_table_unref);
  g_clear_pointer (&name_buf, g_free);

  return pixbuf;

out:
  g_clear_pointer (&colour_map, g_hash_table_unref);
  g_clear_pointer (&name_buf, g_free);

  g_clear_object (&pixbuf);
  return NULL;
}
