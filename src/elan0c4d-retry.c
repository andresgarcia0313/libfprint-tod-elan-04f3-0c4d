/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Recovery layer (elan_session.py): a state machine cannot throw, so each
 * attempt runs under elan_try and the outcome decides: retry after the remedy
 * of the error, or give up. Cancellations and vanished devices are never retried.
 */
#include "elan0c4d-ops.h"

enum { RETRY_ATTEMPT, RETRY_RECOVER, RETRY_STATES };

typedef struct
{
  const char   *name;
  ElanOpNew     op;
  gconstpointer arg;
  gboolean      retry_timeouts;
  guint         attempt;
  ElanRemedy    remedy;
} Retry;

static void
recovered (FpiSsm *ssm, GError *error)
{
  const Retry *r = fpi_ssm_get_data (ssm);

  if (error)
    {
      g_warning ("%s: recovery step failed (%s); the next attempt will retry", r->name, error->message);
      g_error_free (error);
    }
  fpi_ssm_jump_to_state (ssm, RETRY_ATTEMPT);
}

static void
attempted (FpiSsm *ssm, GError *error)
{
  Retry *r = fpi_ssm_get_data (ssm);

  if (error == NULL)
    {
      if (r->attempt > 1)
        g_info ("%s: succeeded at attempt %u/%u", r->name, r->attempt, ELAN_ATTEMPTS);
      fpi_ssm_mark_completed (ssm);
      return;
    }
  r->remedy = elan_error_remedy (error);
  if (r->remedy == ELAN_REMEDY_NONE || (!r->retry_timeouts && elan_error_is_timeout (error)))
    {
      elan_log_error (r->name, error);
      fpi_ssm_mark_failed (ssm, error);
      return;
    }
  g_warning ("%s attempt %u/%u failed: %s", r->name, r->attempt, ELAN_ATTEMPTS, error->message);
  if (r->attempt >= ELAN_ATTEMPTS)
    fpi_ssm_mark_failed (ssm, g_error_new (ELAN_ERROR, ELAN_ERROR_EXHAUSTED,
                                           "%s failed after %u attempts: %s",
                                           r->name, r->attempt, error->message));
  else
    fpi_ssm_jump_to_state_delayed (ssm, RETRY_RECOVER, ELAN_BACKOFF_MS * r->attempt);
  g_error_free (error);
}

static void
retry_handler (FpiSsm *ssm, FpDevice *dev)
{
  Retry *r = fpi_ssm_get_data (ssm);

  if (fpi_ssm_get_cur_state (ssm) == RETRY_ATTEMPT)
    {
      r->attempt++;
      elan_try (ssm, r->op (dev, r->arg), attempted);
      return;
    }
  g_info ("%s: applying remedy %d before attempt %u", r->name, r->remedy, r->attempt + 1);
  elan_try (ssm, elan_recover_ssm_new (dev, r->remedy), recovered);
}

FpiSsm *
elan_retry_ssm_new (FpDevice *dev, const char *name, ElanOpNew op, gconstpointer arg,
                    gboolean retry_timeouts)
{
  FpiSsm *ssm = fpi_ssm_new_full (dev, retry_handler, RETRY_STATES, RETRY_STATES, "elan-retry");
  Retry *r = g_new0 (Retry, 1);

  r->name = name;
  r->op = op;
  r->arg = arg;
  r->retry_timeouts = retry_timeouts;
  fpi_ssm_set_data (ssm, r, g_free);
  return ssm;
}
