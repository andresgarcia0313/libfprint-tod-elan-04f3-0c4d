/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Enroll action: ELAN_ENROLL_STAGES touches (elan0c4d-stages.c), then the
 * comparator builds the template from all of them and the print keeps only
 * that template (keypoints and descriptors, elan0c4d-print.c).
 */
#include "elan0c4d-actions.h"
#include "elan0c4d-enroll.h"

static gboolean
build_template (const ElanEnrollment *e, GBytes **tmpl, GError **error)
{
  ElanFrame frames[ELAN_ENROLL_STAGES];

  if (e == NULL || e->n != ELAN_ENROLL_STAGES)
    {
      g_set_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_GENERAL, "enroll ended with %u of %d stages",
                   e ? e->n : 0, ELAN_ENROLL_STAGES);
      return FALSE;
    }
  for (guint i = 0; i < e->n; i++)
    frames[i] = (ElanFrame) { e->pixels[i], e->background[i], ELAN_WIDTH, ELAN_HEIGHT };
  return elan_match_enroll (frames, e->n, tmpl, error);
}

static void
enroll_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  g_autofree ElanEnrollment *e = g_steal_pointer (&FPI_DEVICE_ELAN0C4D (dev)->enrollment);
  g_autoptr(GBytes) tmpl = NULL;
  FpPrint *print = NULL;

  if (error == NULL && build_template (e, &tmpl, &error))
    {
      fpi_device_get_enroll_data (dev, &print);
      elan_print_store (print, e, tmpl);
      fpi_device_enroll_complete (dev, g_object_ref (print), NULL);
      return;
    }
  elan_log_error ("enroll", error);
  fpi_device_enroll_complete (dev, NULL, elan_error_to_device (error));
}

void
elan_action_enroll (FpDevice *dev)
{
  fpi_ssm_start (elan_stages_ssm_new (dev), enroll_done);
}
