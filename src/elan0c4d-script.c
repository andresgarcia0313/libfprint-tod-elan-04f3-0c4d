/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* A script runs a fixed list of exchanges one after another and stops at the first failure. */
#include "elan0c4d-ops.h"

typedef struct
{
  const ElanCmd *cmds;
  guint          n;
} Script;

static void
script_handler (FpiSsm *ssm, FpDevice *dev)
{
  const Script *script = fpi_ssm_get_data (ssm);
  guint step = fpi_ssm_get_cur_state (ssm);

  if (step >= script->n)
    fpi_ssm_mark_completed (ssm);
  else
    fpi_ssm_start_subsm (ssm, elan_cmd_ssm_new (dev, &script->cmds[step]));
}

FpiSsm *
elan_script_ssm_new (FpDevice *dev, const ElanCmd *cmds, guint n)
{
  FpiSsm *ssm = fpi_ssm_new_full (dev, script_handler, n + 1, n + 1, "elan-script");
  Script *script = g_new0 (Script, 1);

  script->cmds = cmds;
  script->n = n;
  fpi_ssm_set_data (ssm, script, g_free);
  return ssm;
}
