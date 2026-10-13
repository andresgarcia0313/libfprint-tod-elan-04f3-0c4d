/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * elan_try: an FpiSsm child whose failure does NOT fail the parent. The outcome
 * (NULL or an error the callback owns) is handed to `done`, the equivalent of
 * a try/except around one step.
 */
#include "elan0c4d-ops.h"

typedef struct
{
  FpiSsm     *owner;
  FpiSsm     *child;
  ElanTryDone done;
} Try;

static void
try_handler (FpiSsm *ssm, FpDevice *dev)
{
  Try *t = fpi_ssm_get_data (ssm);

  if (fpi_ssm_get_cur_state (ssm) == 0)
    fpi_ssm_start_subsm (ssm, t->child);
  else
    fpi_ssm_mark_completed (ssm);
}

static void
try_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  Try *t = fpi_ssm_get_data (ssm);

  t->done (t->owner, error);
}

void
elan_try (FpiSsm *owner, FpiSsm *child, ElanTryDone done)
{
  FpiSsm *ssm = fpi_ssm_new_full (fpi_ssm_get_device (owner), try_handler, 2, 2, "elan-try");
  Try *t = g_new0 (Try, 1);

  t->owner = owner;
  t->child = child;
  t->done = done;
  fpi_ssm_set_data (ssm, t, g_free);
  fpi_ssm_start (ssm, try_done);
}
