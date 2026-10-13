/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * What an ELAN print holds in fpi-data: (format, width, height, stages,
 * template). The stages are the accepted touches normalized to 8 bits
 * (elan_quality.py normalized), so a later comparator can rebuild its own
 * template from them; the template is the one the comparator built. Data of
 * another format or geometry is refused with FP_DEVICE_ERROR_DATA_INVALID.
 */
#include "elan0c4d-actions.h"
#include "elan0c4d-enroll.h"

#define PRINT_TYPE "(qqqaayay)"

static GVariant *
bytes_variant (gconstpointer data, gsize size)
{
  return g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, data, size, 1);
}

GVariant *
elan_print_data_new (const ElanEnrollment *e, GBytes *tmpl)
{
  GVariantBuilder stages;
  gsize size = 0;
  gconstpointer data = tmpl ? g_bytes_get_data (tmpl, &size) : NULL;

  g_variant_builder_init (&stages, G_VARIANT_TYPE ("aay"));
  for (guint i = 0; i < e->n; i++)
    g_variant_builder_add_value (&stages, bytes_variant (e->normalized[i], ELAN_PIXELS));
  /* No template (or an empty one) is still a valid, empty "ay". */
  return g_variant_new ("(qqq@aay@ay)", ELAN_PRINT_FORMAT, ELAN_WIDTH, ELAN_HEIGHT,
                        g_variant_builder_end (&stages), bytes_variant (data ? data : "", data ? size : 0));
}

static gboolean
invalid (GError **error, const char *why)
{
  g_set_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_DATA_INVALID, "not an ELAN print: %s", why);
  return FALSE;
}

gboolean
elan_print_data_parse (GVariant *data, GPtrArray **frames, GBytes **tmpl, GError **error)
{
  g_autoptr(GVariant) stages = NULL, bytes = NULL;
  guint16 format, width, height;
  gsize size;

  if (data == NULL || !g_variant_is_of_type (data, G_VARIANT_TYPE (PRINT_TYPE)))
    return invalid (error, "no data or another layout");
  g_variant_get (data, "(qqq@aay@ay)", &format, &width, &height, &stages, &bytes);
  if (format != ELAN_PRINT_FORMAT)
    return invalid (error, "unknown format version");
  if (width != ELAN_WIDTH || height != ELAN_HEIGHT || g_variant_n_children (stages) == 0)
    return invalid (error, "wrong geometry or no stages");
  for (gsize i = 0; i < g_variant_n_children (stages); i++)
    {
      g_autoptr(GVariant) stage = g_variant_get_child_value (stages, i);

      if (g_variant_get_size (stage) != ELAN_PIXELS)
        return invalid (error, "stage of the wrong size");
    }
  if (frames)
    {
      *frames = g_ptr_array_new_with_free_func ((GDestroyNotify) g_bytes_unref);
      for (gsize i = 0; i < g_variant_n_children (stages); i++)
        {
          g_autoptr(GVariant) stage = g_variant_get_child_value (stages, i);

          g_ptr_array_add (*frames, g_variant_get_data_as_bytes (stage));
        }
    }
  if (tmpl)
    {
      gconstpointer data = g_variant_get_fixed_array (bytes, &size, 1);

      *tmpl = g_bytes_new (data, size);
    }
  return TRUE;
}

void
elan_print_store (FpPrint *print, const ElanEnrollment *enrollment, GBytes *tmpl)
{
  fpi_print_set_type (print, FPI_PRINT_RAW);
  g_object_set (print, "fpi-data", elan_print_data_new (enrollment, tmpl), NULL);
}

GBytes *
elan_print_load (FpPrint *print, GError **error)
{
  g_autoptr(GVariant) data = NULL;
  GBytes *tmpl = NULL;

  g_object_get (print, "fpi-data", &data, NULL);
  if (!elan_print_data_parse (data, NULL, &tmpl, error))
    return NULL;
  return tmpl;
}
