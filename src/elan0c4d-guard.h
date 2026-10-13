/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Safety checks that need no hardware: forbidden commands and device revision. */
#pragma once

#include <glib.h>

/* TRUE for 42 01 (RUNIAP, jump to the bootloader) and 40 27 (WDTRST, watchdog reset). */
gboolean elan_cmd_is_forbidden (const guint8 *cmd, gsize len);

/* usb_discover score: 100 only for bcdDevice 0x2410, 0 for any other revision. */
gint elan_release_score (guint16 release);
