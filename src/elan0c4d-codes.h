/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Interpretation of the status bytes the MCU sends on EP 0x83 after 40 3e / 40 3f. */
#pragma once

#include <glib.h>

typedef enum {
  ELAN_SCAN_NONE,      /* nothing decisive: 0xFF, unknown bytes -> re-send the command */
  ELAN_SCAN_FOUND,     /* one of the wanted codes came first */
  ELAN_SCAN_KEY_LOST,  /* 0xCC came before any wanted code */
} ElanScan;

/* Bytes are examined in order, like WbfUsbDriver; `found` receives the wanted code. */
ElanScan elan_scan_codes (const guint8 *data, gsize len, const guint8 *wanted, gsize n_wanted,
                          guint8 *found);
