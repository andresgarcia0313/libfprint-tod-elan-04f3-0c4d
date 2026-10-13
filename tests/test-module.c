/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Loads the built module like the TOD loader does and checks what libfprint would see. */
#include <gmodule.h>
#include "drivers_api.h"

static const char *module_path;

static void
test_tod_entry_point (void)
{
  GModule *module = g_module_open (module_path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
  GType (*getter) (void) = NULL;
  FpDeviceClass *cls;
  GType type;

  g_assert_nonnull (module);
  g_assert_true (g_module_symbol (module, "fpi_tod_shared_driver_get_type", (gpointer *) &getter));
  type = getter ();
  g_assert_true (G_TYPE_IS_OBJECT (type));
  g_assert_true (g_type_is_a (type, FP_TYPE_DEVICE));

  cls = g_type_class_ref (type);
  g_assert_cmpstr (cls->id, ==, "elan0c4d-tod");
  g_assert_cmpint (cls->type, ==, FP_DEVICE_TYPE_USB);
  g_assert_cmpint (cls->scan_type, ==, FP_SCAN_TYPE_PRESS);
  g_assert_cmpint (cls->nr_enroll_stages, ==, 12);  /* fprintd shows the progress of each touch */
  g_assert_cmphex (cls->id_table[0].vid, ==, 0x04f3);
  g_assert_cmphex (cls->id_table[0].pid, ==, 0x0c4d);
  g_assert_cmpint (cls->id_table[1].pid, ==, 0);
  g_assert_nonnull (cls->usb_discover);
  g_assert_nonnull (cls->open);
  g_assert_nonnull (cls->close);
  g_assert_nonnull (cls->enroll);
  g_assert_nonnull (cls->verify);
  g_assert_nonnull (cls->identify);
  g_assert_true (cls->features & FP_DEVICE_FEATURE_VERIFY);
  g_assert_true (cls->features & FP_DEVICE_FEATURE_IDENTIFY);
  g_type_class_unref (cls);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_assert_cmpint (argc, ==, 2);
  module_path = argv[1];
  g_test_add_func ("/module/tod-entry-point", test_tod_entry_point);
  return g_test_run ();
}
