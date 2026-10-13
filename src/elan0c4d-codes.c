/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "elan0c4d-codes.h"
#include "elan0c4d-proto.h"

ElanScan
elan_scan_codes (const guint8 *data, gsize len, const guint8 *wanted, gsize n_wanted, guint8 *found)
{
  for (gsize i = 0; i < len; i++)
    {
      for (gsize j = 0; j < n_wanted; j++)
        if (data[i] == wanted[j])
          {
            *found = data[i];
            return ELAN_SCAN_FOUND;
          }
      if (data[i] == ELAN_CODE_KEY_LOST)
        return ELAN_SCAN_KEY_LOST;
    }
  return ELAN_SCAN_NONE;
}
