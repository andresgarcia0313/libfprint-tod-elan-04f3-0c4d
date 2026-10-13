/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * What an ELAN print holds in fpi-data: (format, width, height, stages,
 * template). Only the template the comparator built is kept: the touches
 * themselves are fingerprint images and never leave the driver (data
 * minimization; fprintd stores prints under /var/lib/fprint). Data of another
 * format or geometry, or without a template, is refused with
 * FP_DEVICE_ERROR_DATA_INVALID.
 */
#include "elan0c4d-actions.h"
#include "elan0c4d-enroll.h"

#define PRINT_TYPE "(qqqqay)"

GVariant *
elan_print_data_new (const ElanEnrollment *e, GBytes *tmpl)
{
  gsize size = 0;
  gconstpointer data = tmpl ? g_bytes_get_data (tmpl, &size) : NULL;

  return g_variant_new ("(qqqq@ay)", ELAN_PRINT_FORMAT, ELAN_WIDTH, ELAN_HEIGHT, (guint16) e->n,
                        g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, data ? data : "", data ? size : 0, 1));
}

static gboolean
invalid (GError **error, const char *why)
{
  g_set_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_DATA_INVALID, "not an ELAN print: %s", why);
  return FALSE;
}

gboolean
elan_print_data_parse (GVariant *data, GBytes **tmpl, GError **error)
{
  g_autoptr(GVariant) bytes = NULL;
  guint16 format, width, height, stages;
  gconstpointer raw;
  gsize size = 0;

  if (data == NULL || !g_variant_is_of_type (data, G_VARIANT_TYPE (PRINT_TYPE)))
    return invalid (error, "no data or another layout");
  g_variant_get (data, "(qqqq@ay)", &format, &width, &height, &stages, &bytes);
  if (format != ELAN_PRINT_FORMAT)
    return invalid (error, "unknown format version");
  if (width != ELAN_WIDTH || height != ELAN_HEIGHT || stages == 0)
    return invalid (error, "wrong geometry or no stages");
  raw = g_variant_get_fixed_array (bytes, &size, 1);
  if (size == 0)
    return invalid (error, "empty template");
  if (tmpl)
    *tmpl = g_bytes_new (raw, size);
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
  if (!elan_print_data_parse (data, &tmpl, error))
    return NULL;
  return tmpl;
}
