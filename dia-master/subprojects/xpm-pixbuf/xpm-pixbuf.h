/*
 * © 2024 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

G_MODULE_EXPORT
GdkPixbuf *xpm_pixbuf_load (const char *const restrict data[const restrict]);

G_END_DECLS
