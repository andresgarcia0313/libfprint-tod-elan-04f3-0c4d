/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "elan0c4d-guard.h"
#include "elan0c4d-proto.h"

gboolean
elan_cmd_is_forbidden (const guint8 *cmd, gsize len)
{
  if (cmd == NULL || len < 2)
    return FALSE;
  return (cmd[0] == 0x42 && cmd[1] == 0x01) || (cmd[0] == 0x40 && cmd[1] == 0x27);
}

gint
elan_release_score (guint16 release)
{
  return release == ELAN_RELEASE ? 100 : 0;
}
