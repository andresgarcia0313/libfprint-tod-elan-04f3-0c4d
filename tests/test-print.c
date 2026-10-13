/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* What the print keeps: format version, geometry, stage count and the template; never the images. */
#include "elan0c4d-actions.h"
#include "elan0c4d-enroll.h"

static ElanEnrollment *
three_stages (void)
{
  ElanEnrollment *e = g_new0 (ElanEnrollment, 1);

  e->n = 3;
  for (guint s = 0; s < e->n; s++)
    for (guint i = 0; i < ELAN_PIXELS; i++)
      e->normalized[s][i] = (i * (s + 1)) & 0xFF;
  return e;
}

static void
test_round_trip_keeps_no_image (void)
{
  g_autofree ElanEnrollment *e = three_stages ();
  g_autoptr(GBytes) tmpl = g_bytes_new_static ("tmpl", 4), back = NULL;
  g_autoptr(GVariant) data = g_variant_ref_sink (elan_print_data_new (e, tmpl));
  g_autoptr(GError) error = NULL;
  guint16 format, stages;

  g_assert_cmpstr (g_variant_get_type_string (data), ==, "(qqqqay)");
  g_variant_get_child (data, 0, "q", &format);
  g_variant_get_child (data, 3, "q", &stages);
  g_assert_cmpuint (format, ==, ELAN_PRINT_FORMAT);
  g_assert_cmpuint (stages, ==, 3);
  g_assert_cmpuint (g_variant_get_size (data), <, ELAN_PIXELS);  /* not even one touch fits */
  g_assert_true (elan_print_data_parse (data, &back, &error));
  g_assert_no_error (error);
  g_assert_true (g_bytes_equal (tmpl, back));
}

static void
assert_refused (GVariant *data)
{
  g_autoptr(GVariant) owned = data ? g_variant_ref_sink (data) : NULL;
  g_autoptr(GBytes) tmpl = NULL;
  g_autoptr(GError) error = NULL;

  g_assert_false (elan_print_data_parse (owned, &tmpl, &error));
  g_assert_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_DATA_INVALID);
  g_assert_null (tmpl);
}

static GVariant *
variant_with (guint16 format, guint16 width, guint16 stages, gsize tmpl_size)
{
  return g_variant_new ("(qqqq@ay)", format, width, ELAN_HEIGHT, stages,
                        g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, "tmpl", tmpl_size, 1));
}

static void
test_other_formats_refused (void)
{
  g_autofree guint8 *px = g_malloc0 (ELAN_PIXELS);
  GVariant *stage = g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, px, ELAN_PIXELS, 1);

  assert_refused (NULL);
  assert_refused (g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, "raw", 3, 1));  /* 0.1.0: bare template */
  assert_refused (g_variant_new ("(qqq@aay@ay)", 1, ELAN_WIDTH, ELAN_HEIGHT,     /* format 1 kept images */
                                 g_variant_new_array (NULL, &stage, 1),
                                 g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, "", 0, 1)));
  assert_refused (variant_with (ELAN_PRINT_FORMAT + 1, ELAN_WIDTH, 3, 4));
  assert_refused (variant_with (ELAN_PRINT_FORMAT, ELAN_WIDTH + 1, 3, 4));
  assert_refused (variant_with (ELAN_PRINT_FORMAT, ELAN_WIDTH, 0, 4));
  assert_refused (variant_with (ELAN_PRINT_FORMAT, ELAN_WIDTH, 3, 0));          /* nothing to match against */
}

/* The FpPrint carries the data as a raw print and gives the template back. */
static void
test_print_store_and_load (void)
{
  g_autofree ElanEnrollment *e = three_stages ();
  g_autoptr(FpPrint) print = g_object_new (FP_TYPE_PRINT, "driver", "elan0c4d-tod", "device-id", "sim", NULL);
  g_autoptr(GBytes) tmpl = g_bytes_new_static ("features", 8), back = NULL;
  g_autoptr(GError) error = NULL;

  elan_print_store (print, e, tmpl);
  back = elan_print_load (print, &error);
  g_assert_no_error (error);
  g_assert_true (g_bytes_equal (tmpl, back));
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/print/round-trip-keeps-no-image", test_round_trip_keeps_no_image);
  g_test_add_func ("/print/other-formats-refused", test_other_formats_refused);
  g_test_add_func ("/print/store-and-load", test_print_store_and_load);
  return g_test_run ();
}
