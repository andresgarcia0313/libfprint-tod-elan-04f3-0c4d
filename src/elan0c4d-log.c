/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Log helpers: every command, answer, recovery and error goes through g_debug/g_info/g_warning. */
#include <gio/gio.h>
#include "elan0c4d-ops.h"

void
elan_log_hex (const char *dir, guint8 ep, const guint8 *buf, gsize len)
{
  g_autoptr(GString) hex = g_string_new (NULL);

  for (gsize i = 0; i < MIN (len, 32); i++)
    g_string_append_printf (hex, "%02x", buf[i]);
  g_debug ("%s ep=0x%02x len=%" G_GSIZE_FORMAT " %s%s", dir, ep, len, hex->str, len > 32 ? "..." : "");
}

void
elan_log_error (const char *what, const GError *error)
{
  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    g_info ("%s: cancelled", what);
  else if (elan_error_is_timeout (error))  /* expected while waiting; callers report real failures */
    g_debug ("%s: %s", what, error->message);
  else
    g_warning ("%s: %s", what, error->message);
}
