/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* What the print keeps: format version, geometry, the 8-bit stages and the template. */
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
test_round_trip (void)
{
  g_autofree ElanEnrollment *e = three_stages ();
  g_autoptr(GBytes) tmpl = g_bytes_new_static ("tmpl", 4), back = NULL;
  g_autoptr(GVariant) data = g_variant_ref_sink (elan_print_data_new (e, tmpl));
  g_autoptr(GPtrArray) frames = NULL;
  g_autoptr(GError) error = NULL;
  guint16 format;

  g_assert_cmpstr (g_variant_get_type_string (data), ==, "(qqqaayay)");
  g_variant_get_child (data, 0, "q", &format);
  g_assert_cmpuint (format, ==, ELAN_PRINT_FORMAT);
  g_assert_true (elan_print_data_parse (data, &frames, &back, &error));
  g_assert_no_error (error);
  g_assert_true (g_bytes_equal (tmpl, back));
  g_assert_cmpuint (frames->len, ==, 3);
  for (guint s = 0; s < 3; s++)
    {
      gsize size;
      gconstpointer px = g_bytes_get_data (g_ptr_array_index (frames, s), &size);

      g_assert_cmpmem (px, size, e->normalized[s], ELAN_PIXELS);
    }
}

static void
assert_refused (GVariant *data)
{
  g_autoptr(GVariant) owned = data ? g_variant_ref_sink (data) : NULL;
  g_autoptr(GBytes) tmpl = NULL;
  g_autoptr(GError) error = NULL;

  g_assert_false (elan_print_data_parse (owned, NULL, &tmpl, &error));
  g_assert_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_DATA_INVALID);
  g_assert_null (tmpl);
}

static GVariant *
variant_with (guint16 format, guint16 width, guint stage_size)
{
  g_autofree guint8 *px = g_malloc0 (stage_size);
  GVariant *stage = g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, px, stage_size, 1);

  return g_variant_new ("(qqq@aay@ay)", format, width, ELAN_HEIGHT, g_variant_new_array (NULL, &stage, 1),
                        g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, "", 0, 1));
}

static void
test_other_formats_refused (void)
{
  assert_refused (NULL);
  assert_refused (g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, "raw", 3, 1));  /* 0.1.0: bare template */
  assert_refused (variant_with (ELAN_PRINT_FORMAT + 1, ELAN_WIDTH, ELAN_PIXELS));
  assert_refused (variant_with (ELAN_PRINT_FORMAT, ELAN_WIDTH + 1, ELAN_PIXELS));
  assert_refused (variant_with (ELAN_PRINT_FORMAT, ELAN_WIDTH, ELAN_PIXELS - 1));
}

/* The FpPrint carries the data as a raw print; no template is stored as an empty one. */
static void
test_print_store_and_load (void)
{
  g_autofree ElanEnrollment *e = three_stages ();
  g_autoptr(FpPrint) print = g_object_new (FP_TYPE_PRINT, "driver", "elan0c4d-tod", "device-id", "sim", NULL);
  g_autoptr(GBytes) back = NULL;
  g_autoptr(GError) error = NULL;

  elan_print_store (print, e, NULL);
  back = elan_print_load (print, &error);
  g_assert_no_error (error);
  g_assert_cmpuint (g_bytes_get_size (back), ==, 0);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/print/round-trip", test_round_trip);
  g_test_add_func ("/print/other-formats-refused", test_other_formats_refused);
  g_test_add_func ("/print/store-and-load", test_print_store_and_load);
  return g_test_run ();
}
