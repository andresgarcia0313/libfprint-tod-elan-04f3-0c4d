/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Enroll in several touches against the simulated sensor: progress, retries and the comparator. */
#include "sim.h"
#include "elan0c4d-enroll.h"

#define LIFT " 0009"  /* one frame read while waiting for the lift: the finger is already gone */

static void
test_stages_report_progress (void)
{
  SimImage touches[ELAN_ENROLL_STAGES];
  SimSensor s;
  g_autoptr(GString) wire = g_string_new (W_PREPARE " 0009");
  FpiDeviceElan0c4d *dev;
  g_autoptr(GError) error = NULL;

  for (guint i = 0; i < ELAN_ENROLL_STAGES; i++)
    touches[i] = i % 2 ? SIM_MOVED : SIM_FINGER;
  for (guint i = 1; i < ELAN_ENROLL_STAGES; i++)
    g_string_append (wire, LIFT " " W_PREPARE " 0009");
  dev = sim_start_touches (&s, touches, ELAN_ENROLL_STAGES);
  error = sim_run (elan_stages_ssm_new (FP_DEVICE (dev)));
  g_assert_no_error (error);
  sim_assert_progress (ELAN_ENROLL_STAGES, G_MAXUINT, -1);
  g_assert_cmpstr (s.wire->str, ==, wire->str);
  g_assert_cmpuint (dev->enrollment->n, ==, ELAN_ENROLL_STAGES);
  g_assert_cmpfloat (elan_quality_similarity (dev->enrollment->normalized[0], dev->enrollment->normalized[1],
                                              ELAN_PIXELS), <, ELAN_ENROLL_SIMILAR_MAX);
  g_assert_cmpuint (sim_warnings (), ==, 0);
  sim_end (dev);
}

/* A touch that is not a new stage is reported as a retry and one more touch is needed. */
static void
check_retry (SimImage rejected, gint retry)
{
  SimImage touches[ELAN_ENROLL_STAGES + 1] = { SIM_FINGER, rejected };
  SimSensor s;
  FpiDeviceElan0c4d *dev;
  g_autoptr(GError) error = NULL;

  for (guint i = 2; i < G_N_ELEMENTS (touches); i++)
    touches[i] = i % 2 ? SIM_FINGER : SIM_MOVED;
  dev = sim_start_touches (&s, touches, G_N_ELEMENTS (touches));
  error = sim_run (elan_stages_ssm_new (FP_DEVICE (dev)));
  g_assert_no_error (error);
  sim_assert_progress (ELAN_ENROLL_STAGES + 1, 1, retry);
  g_assert_cmpuint (dev->enrollment->n, ==, ELAN_ENROLL_STAGES);
  sim_end (dev);
}

static void
test_same_touch_is_retried (void)
{
  check_retry (SIM_FINGER, FP_DEVICE_RETRY_REMOVE_FINGER);
}

static void
test_partial_touch_is_retried (void)
{
  check_retry (SIM_PARTIAL, FP_DEVICE_RETRY_CENTER_FINGER);
}

/* The action hands every stage to the comparator; the stub refuses and the stages are freed. */
static void
test_action_ends_in_the_comparator (void)
{
  SimImage touches[ELAN_ENROLL_STAGES];
  SimSensor s;
  FpiDeviceElan0c4d *dev;
  const GError *error;

  for (guint i = 0; i < ELAN_ENROLL_STAGES; i++)
    touches[i] = i % 2 ? SIM_MOVED : SIM_FINGER;
  dev = sim_start_touches (&s, touches, ELAN_ENROLL_STAGES);
  error = sim_run_enroll (dev);  /* g_assert_error evaluates its argument more than once */
  g_assert_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_NOT_SUPPORTED);
  sim_assert_progress (ELAN_ENROLL_STAGES, G_MAXUINT, -1);
  g_assert_null (dev->enrollment);
  sim_end (dev);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/sim/enroll/stages-report-progress", test_stages_report_progress);
  g_test_add_func ("/sim/enroll/same-touch-is-retried", test_same_touch_is_retried);
  g_test_add_func ("/sim/enroll/partial-touch-is-retried", test_partial_touch_is_retried);
  g_test_add_func ("/sim/enroll/action-ends-in-the-comparator", test_action_ends_in_the_comparator);
  return g_test_run ();
}
