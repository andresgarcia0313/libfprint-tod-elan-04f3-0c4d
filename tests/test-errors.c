/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Error remedies, conversion to FpDevice errors and the comparator stub. */
#include <gio/gio.h>
#include <gusb.h>
#include "fpi-device.h"
#include "elan0c4d-error.h"
#include "elan0c4d-match.h"

static ElanRemedy
remedy_of (GQuark domain, gint code)
{
  g_autoptr(GError) error = g_error_new_literal (domain, code, "test");

  return elan_error_remedy (error);
}

static void
test_remedies (void)
{
  g_assert_cmpint (remedy_of (ELAN_ERROR, ELAN_ERROR_TIMEOUT), ==, ELAN_REMEDY_REINIT);
  g_assert_cmpint (remedy_of (ELAN_ERROR, ELAN_ERROR_PROTOCOL), ==, ELAN_REMEDY_REINIT);
  g_assert_cmpint (remedy_of (ELAN_ERROR, ELAN_ERROR_CALIBRATION), ==, ELAN_REMEDY_REINIT);
  g_assert_cmpint (remedy_of (ELAN_ERROR, ELAN_ERROR_KEY_LOST), ==, ELAN_REMEDY_REKEY);
  g_assert_cmpint (remedy_of (ELAN_ERROR, ELAN_ERROR_FORBIDDEN), ==, ELAN_REMEDY_NONE);
  g_assert_cmpint (remedy_of (ELAN_ERROR, ELAN_ERROR_EXHAUSTED), ==, ELAN_REMEDY_NONE);
  g_assert_cmpint (remedy_of (G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_TIMED_OUT), ==, ELAN_REMEDY_REINIT);
  g_assert_cmpint (remedy_of (G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_IO), ==, ELAN_REMEDY_REOPEN);
  g_assert_cmpint (remedy_of (G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_NO_DEVICE), ==, ELAN_REMEDY_NONE);
  g_assert_cmpint (remedy_of (G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_BUSY), ==, ELAN_REMEDY_NONE);
  g_assert_cmpint (remedy_of (G_IO_ERROR, G_IO_ERROR_CANCELLED), ==, ELAN_REMEDY_NONE);
}

static void
test_error_to_device (void)
{
  GError *gone = g_error_new_literal (G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_NO_DEVICE, "unplugged");
  GError *proto = g_error_new_literal (ELAN_ERROR, ELAN_ERROR_PROTOCOL, "short answer");
  GError *cancelled = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "cancelled");
  g_autoptr(GError) out = elan_error_to_device (gone);

  g_assert_error (out, FP_DEVICE_ERROR, FP_DEVICE_ERROR_REMOVED);
  g_clear_error (&out);
  out = elan_error_to_device (proto);
  g_assert_error (out, FP_DEVICE_ERROR, FP_DEVICE_ERROR_PROTO);
  g_assert_nonnull (strstr (out->message, "short answer"));
  g_clear_error (&out);
  out = elan_error_to_device (cancelled);
  g_assert_error (out, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_null (elan_error_to_device (NULL));
}

static void
test_match_stub (void)
{
  guint16 px[4] = { 0 };
  ElanFrame frames[2] = { { px, px, 2, 2 }, { px, px, 2, 2 } };
  g_autoptr(GBytes) tmpl = NULL;
  g_autoptr(GError) error = NULL;
  gboolean matched = TRUE;

  g_assert_false (elan_match_enroll (frames, G_N_ELEMENTS (frames), &tmpl, &error));
  g_assert_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_NOT_SUPPORTED);
  g_assert_null (tmpl);
  g_clear_error (&error);
  g_assert_false (elan_match_verify (&frames[0], NULL, &matched, &error));
  g_assert_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_NOT_SUPPORTED);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/errors/remedies", test_remedies);
  g_test_add_func ("/errors/error-to-device", test_error_to_device);
  g_test_add_func ("/errors/match-stub", test_match_stub);
  return g_test_run ();
}
