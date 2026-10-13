/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Templates travel inside the FpPrint as a raw byte array ("ay") in the fpi-data property. */
#include "elan0c4d-actions.h"

void
elan_print_store (FpPrint *print, GBytes *tmpl)
{
  gsize size;
  gconstpointer data = g_bytes_get_data (tmpl, &size);

  fpi_print_set_type (print, FPI_PRINT_RAW);
  g_object_set (print, "fpi-data", g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, data, size, 1), NULL);
}

GBytes *
elan_print_load (FpPrint *print)
{
  g_autoptr(GVariant) data = NULL;
  gconstpointer bytes;
  gsize size = 0;

  g_object_get (print, "fpi-data", &data, NULL);
  if (data == NULL || !g_variant_is_of_type (data, G_VARIANT_TYPE_BYTESTRING))
    return NULL;
  bytes = g_variant_get_fixed_array (data, &size, 1);
  return g_bytes_new (bytes, size);
}
