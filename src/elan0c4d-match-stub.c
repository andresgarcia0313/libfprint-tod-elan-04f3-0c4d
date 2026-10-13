/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Placeholder comparator: every operation reports FP_DEVICE_ERROR_NOT_SUPPORTED. */
#include "fpi-device.h"
#include "elan0c4d-match.h"

static gboolean
not_supported (GError **error, const char *what)
{
  g_set_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_NOT_SUPPORTED,
               "no fingerprint comparator is built in: %s is not supported", what);
  return FALSE;
}

gboolean
elan_match_enroll (const ElanFrame *frame, GBytes **tmpl, GError **error)
{
  return not_supported (error, "enroll");
}

gboolean
elan_match_verify (const ElanFrame *frame, GBytes *tmpl, gboolean *matched, GError **error)
{
  return not_supported (error, "verify");
}
